/*
  SIH26178 / PRAKRITI - Node 1 (Hydro-Meteorological Intelligence)
  ------------------------------------------------------------------
  Final integrated sketch: real sensor drivers + feature extraction +
  on-device TinyML inference + Confidence/Risk Engine + LoRa transmit.

  HARDWARE (per PRAKRITI_Node1_Node4_Wiring_Guide.pdf):
    ESP32-S3-N16R8
    AJ-SR04M       ultrasonic water level   TRIG=GPIO6  ECHO=GPIO7 (via divider)
    GY-BME280      temp/humidity/pressure  SDA=GPIO4   SCL=GPIO5  (I2C 0x76/0x77)
    Soil moisture resistive (analog)       AOUT=GPIO1
    Raindrop sensor (analog)               AOUT=GPIO2
    LoRa-02 SX1278 433MHz (SPI)            SCK=12 MISO=13 MOSI=11 NSS=10 RST=9 DIO0=8

  LIBRARIES (Arduino IDE Library Manager):
    - RadioLib
    - Adafruit BME280 Library  (+ Adafruit Unified Sensor, installed automatically)
    - Chirale_TensorFlowLite
    - ArduTFLite

  Put model_data.h in this same folder before compiling.
*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <RadioLib.h>
#include <ArduTFLite.h>
#include "model_data.h"

// ===================================================================
//  PIN MAP
// ===================================================================
#define LORA_SCK    12
#define LORA_MISO   13
#define LORA_MOSI   11
#define LORA_NSS    10
#define LORA_RESET  9
#define LORA_DIO0   8

#define BME_SDA     4
#define BME_SCL     5
#define BME_ADDR_PRIMARY   0x76
#define BME_ADDR_SECONDARY 0x77

#define TRIG_PIN    6
#define ECHO_PIN    7

#define SOIL_PIN    1   // ADC1_CH0 - not fed to this model, logged for Node 3 / future use
#define RAIN_PIN    2   // ADC1_CH1 - analog raindrop reading

// ===================================================================
//  CALIBRATION CONSTANTS
// ===================================================================
const float MOUNT_HEIGHT_CM = 100.0f;
const float MAX_ULTRASONIC_RANGE_CM = 300.0f;

const bool  RAIN_INVERTED = true;
const float RAIN_PROXY_MAX = 35.0f;

// ===================================================================
//  GLOBALS & STRUCTURES
// ===================================================================
Adafruit_BME280 bme;
bool bmeOk = false;

struct FeatureVec {
    float mean;
    float rms;
    float var;
    float rate;
    float trend;
    float persistence;
};

SPIClass loraSPI(FSPI);
SPISettings loraSPISettings(2000000, MSBFIRST, SPI_MODE0);
SX1278 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RESET, RADIOLIB_NC, loraSPI, loraSPISettings);
bool loraOk = false;

constexpr int kTensorArenaSize = 16 * 1024;
alignas(16) uint8_t tensorArena[kTensorArenaSize];
bool modelOk = false;

#define WINDOW_SIZE 10
float waterBuf[WINDOW_SIZE];
float rainBuf[WINDOW_SIZE];
float pressureBuf[WINDOW_SIZE];
int sampleCount = 0;

const char* STATE_NAMES[4] = {"NORMAL", "WATCH", "WARNING", "CRITICAL"};
int currentStateIdx = 0;      // starts at NORMAL
int downgradeStreak = 0;
uint32_t seqNum = 0;
uint32_t lastSampleMillis = 0;
uint32_t sampleIntervalMs = 5000; // adapts with risk state

// ===================================================================
//  SENSOR READ FUNCTIONS
// ===================================================================

float readWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  float distanceCm;
  if (duration == 0) {
    distanceCm = MAX_ULTRASONIC_RANGE_CM;
  } else {
    distanceCm = (duration * 0.0343f) / 2.0f;
    if (distanceCm > MAX_ULTRASONIC_RANGE_CM) distanceCm = MAX_ULTRASONIC_RANGE_CM;
  }

  float level = MOUNT_HEIGHT_CM - distanceCm;
  if (level < 0.0f) level = 0.0f;
  return level;
}

float readRainfall() {
  int raw = analogRead(RAIN_PIN);          // 0-4095 on ESP32-S3 (12-bit ADC)
  float frac = raw / 4095.0f;
  if (RAIN_INVERTED) frac = 1.0f - frac;   // dry=HIGH/wet=LOW boards: invert
  return frac * RAIN_PROXY_MAX;
}

float readPressure() {
  if (!bmeOk) return 1013.0f;
  return bme.readPressure() / 100.0f;
}

float readSoilMoistureRaw() { return analogRead(SOIL_PIN); }
float readTemperatureC()    { return bmeOk ? bme.readTemperature() : NAN; }
float readHumidityPct()     { return bmeOk ? bme.readHumidity() : NAN; }

// ===================================================================
//  ROLLING WINDOW + FEATURE EXTRACTION
// ===================================================================

void pushSample(float* buf, float value) {
  if (sampleCount < WINDOW_SIZE) {
    buf[sampleCount] = value;
  } else {
    memmove(buf, buf + 1, (WINDOW_SIZE - 1) * sizeof(float));
    buf[WINDOW_SIZE - 1] = value;
  }
}

FeatureVec computeFeatures(const float* buf, int n) {
  FeatureVec f{};
  if (n <= 0) return f;

  float sum = 0.0f, sumSq = 0.0f;
  for (int i = 0; i < n; i++) { 
    sum += buf[i]; 
    sumSq += buf[i] * buf[i]; 
  }
  f.mean = sum / n;
  f.rms = sqrtf(sumSq / n);

  float varSum = 0.0f;
  for (int i = 0; i < n; i++) { 
    float d = buf[i] - f.mean; 
    varSum += d * d; 
  }
  f.var = varSum / n; // population variance

  f.rate = (buf[n - 1] - buf[0]) / n;

  // Least-squares linear regression (slope / trend)
  float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumXX = 0.0f;
  for (int i = 0; i < n; i++) {
    float x = (float)i, y = buf[i];
    sumX += x; 
    sumY += y; 
    sumXY += x * y; 
    sumXX += x * x;
  }
  float denom = n * sumXX - sumX * sumX;
  f.trend = (fabsf(denom) < 1e-9f) ? 0.0f : (n * sumXY - sumX * sumY) / denom;

  // Persistence calculation
  int trendSign = (f.trend > 0.0f) ? 1 : ((f.trend < 0.0f) ? -1 : 0);
  int matchCount = 0;
  for (int i = 1; i < n; i++) {
    float diff = buf[i] - buf[i - 1];
    int diffSign = (diff > 0.0f) ? 1 : ((diff < 0.0f) ? -1 : 0);
    if (diffSign == trendSign) matchCount++;
  }
  f.persistence = (n > 1) ? ((float)matchCount / (n - 1)) : 0.0f;

  return f;
}

// ===================================================================
//  CONFIDENCE + RISK ENGINE
// ===================================================================

int riskEngineTick(int modelClassIdx, float waterTrend, float rainMean) {
  int candidate = modelClassIdx;

  bool physicallyPlausible = (waterTrend > 0.05f) || (rainMean > 5.0f);
  if (candidate >= 2 && !physicallyPlausible) {
    candidate = 1; // Cap at WATCH if physical threshold isn't met
  }

  if (candidate > currentStateIdx) {
    currentStateIdx = candidate;
    downgradeStreak = 0;
  } else if (candidate < currentStateIdx) {
    downgradeStreak++;
    if (downgradeStreak >= 3) {
      currentStateIdx = candidate;
      downgradeStreak = 0;
    }
  } else {
    downgradeStreak = 0;
  }
  return currentStateIdx;
}

uint32_t intervalForState(int stateIdx) {
  switch (stateIdx) {
    case 0: return 5000;  // NORMAL
    case 1: return 2000;  // WATCH
    case 2: return 1000;  // WARNING
    default: return 250;  // CRITICAL
  }
}

// ===================================================================
//  SETUP
// ===================================================================

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("=== PRAKRITI Node 1 - Risk Engine booting ==="));

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  analogReadResolution(12);

  Wire.begin(BME_SDA, BME_SCL);
  bmeOk = bme.begin(BME_ADDR_PRIMARY);
  if (!bmeOk) bmeOk = bme.begin(BME_ADDR_SECONDARY);
  Serial.println(bmeOk ? F("BME280: OK") : F("BME280: NOT FOUND (using fallback pressure)"));

  loraSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  int loraState = radio.begin(433.0);
  loraOk = (loraState == RADIOLIB_ERR_NONE);
  Serial.print(F("LoRa init: "));
  Serial.println(loraOk ? F("OK") : String(loraState));

  modelOk = modelInit(g_risk_model, tensorArena, kTensorArenaSize);
  Serial.println(modelOk ? F("TinyML model: OK") : F("TinyML model: FAILED - check tensor arena size"));

  Serial.println(F("Setup complete. Sampling every 5s until risk rises.\n"));
  lastSampleMillis = millis() - sampleIntervalMs;
}

// ===================================================================
//  MAIN LOOP
// ===================================================================

void loop() {
  uint32_t now = millis();
  if (now - lastSampleMillis < sampleIntervalMs) return;
  lastSampleMillis = now;

  float water = readWaterLevel();
  float rain = readRainfall();
  float pressure = readPressure();
  float soilRaw = readSoilMoistureRaw();
  float tempC = readTemperatureC();
  float humidity = readHumidityPct();

  pushSample(waterBuf, water);
  pushSample(rainBuf, rain);
  pushSample(pressureBuf, pressure);
  if (sampleCount < WINDOW_SIZE) sampleCount++;

  Serial.printf("[%lu] water=%.1fcm rain=%.1f pres=%.1fhPa soilRaw=%.0f temp=%.1fC hum=%.1f%%\n",
                (unsigned long)now, water, rain, pressure, soilRaw, tempC, humidity);

  if (sampleCount < WINDOW_SIZE) {
    Serial.printf("  filling window: %d/%d samples\n", sampleCount, WINDOW_SIZE);
    return;
  }

  FeatureVec fw = computeFeatures(waterBuf, WINDOW_SIZE);
  FeatureVec fr = computeFeatures(rainBuf, WINDOW_SIZE);
  FeatureVec fp = computeFeatures(pressureBuf, WINDOW_SIZE);

  float raw[18] = {
    fw.mean, fw.rms, fw.var, fw.rate, fw.trend, fw.persistence,
    fr.mean, fr.rms, fr.var, fr.rate, fr.trend, fr.persistence,
    fp.mean, fp.rms, fp.var, fp.rate, fp.trend, fp.persistence
  };

  if (!modelOk) {
    Serial.println(F("  model not initialized, skipping inference"));
    return;
  }

  for (int i = 0; i < kNumFeatures; i++) {
    float normalized = (raw[i] - kFeatureMean[i]) / kFeatureStd[i];
    modelSetInput(normalized, i);
  }

  if (!modelRunInference()) {
    Serial.println(F("  inference failed"));
    return;
  }

  int bestIdx = 0;
  float bestProb = -1.0f;
  float probs[4];
  for (int i = 0; i < 4; i++) {
    probs[i] = modelGetOutput(i);
    if (probs[i] > bestProb) { bestProb = probs[i]; bestIdx = i; }
  }

  int finalStateIdx = riskEngineTick(bestIdx, fw.trend, fr.mean);
  sampleIntervalMs = intervalForState(finalStateIdx);

  Serial.printf("  model=%s (%.0f%%)  ->  RISK STATE = %s\n",
                STATE_NAMES[bestIdx], bestProb * 100.0f, STATE_NAMES[finalStateIdx]);

  seqNum++;
  char payload[160];
  snprintf(payload, sizeof(payload), "%s,%.2f,%.1f,%.1f,%.1f,%.1f,%.1f,%.0f,%lu",
           STATE_NAMES[finalStateIdx], bestProb, water, rain, pressure,
           tempC, humidity, soilRaw, (unsigned long)seqNum);

  if (loraOk) {
    int txState = radio.transmit(payload);
    Serial.print(F("  LoRa TX: "));
    Serial.println(txState == RADIOLIB_ERR_NONE ? F("sent") : String(txState));
  }
}