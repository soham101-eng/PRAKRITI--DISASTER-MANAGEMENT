  // =======================================================================
// PRAKRITI - NODE 1 (Sensor Node) - ESP32-S3-N16R8
// Ultrasonic water level (AJ-SR04M) + GY-BME280 + soil moisture + raindrop
// -> DSP pipeline (signal conditioning -> features -> baseline -> temporal
//    -> fusion) -> risk state -> LoRa send to Node 4
// =======================================================================

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <RadioLib.h>
#include "SensorTypes.h"
#include "signal_conditioning.h"
#include "feature_extraction.h"
#include "adaptive_baseline.h"
#include "temporal_analysis.h"
#include "sensor_fusion.h"
#include "LoRaPacket.h"

// ----------------------- PIN MAP (from your wiring guide) -----------------------
// LoRa-02 SX1278 (SPI)
#define LORA_SCK    12
#define LORA_MISO   13
#define LORA_MOSI   11
#define LORA_NSS    10
#define LORA_RESET  9
#define LORA_DIO0   8

// GY-BME280 (I2C)
#define BME_SDA     4
#define BME_SCL     5
#define BME_ADDRESS 0x76   // change to 0x77 if begin() fails and your module's SDO is pulled high

// AJ-SR04M ultrasonic water level (Trig/Echo mode)
#define TRIG_PIN    6
#define ECHO_PIN    7      // wired through the 1k/2k divider per your guide

// Resistive soil moisture (analog)
#define SOIL_PIN    1

// Raindrop sensor (analog) - the one part not in the original wiring guide
#define RAIN_PIN    2

// ----------------------- CALIBRATION - CHECK THESE ON YOUR HARDWARE -----------------------
// Ultrasonic: distance measured is from the sensor DOWN to the water
// surface, so it DECREASES as water rises. SENSOR_HEIGHT_CM is the
// distance from the sensor to your reference "zero water" point
// (e.g. the riverbed, or the bottom of a test container). Measure this
// once you mount the sensor and update it here.
#define SENSOR_HEIGHT_CM 100.0f

// Soil moisture: raw ADC reading (0-4095) at fully DRY and fully WET.
// To calibrate: upload this sketch, open Serial Monitor, note the raw
// value with the sensor in open air (dry) and dipped in water (wet),
// then replace these two numbers. Until then this is an approximation.
#define SOIL_DRY_RAW 3000
#define SOIL_WET_RAW 1200

// Raindrop sensor: same idea - most of these boards read a HIGH raw
// value when dry and a LOWER raw value as the board gets wetter /
// rain intensifies. Calibrate the same way: dry reading vs. a few
// drops of water on the sensor plate.
#define RAIN_DRY_RAW 3200
#define RAIN_WET_RAW 800

// ----------------------- LoRa settings - MUST MATCH NODE 4 EXACTLY -----------------------
#define LORA_FREQ_MHZ     433.0
#define LORA_BANDWIDTH    125.0
#define LORA_SPREADING    9
#define LORA_CODING_RATE  7
#define LORA_SYNC_WORD    0x12
#define LORA_TX_POWER     17

Adafruit_BME280 bme;
SX1278 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RESET, RADIOLIB_NC);

static bool hazard_active = false;
static uint32_t packet_seq = 0;
static bool bme_ok = false;

// explicit forward declarations - written by hand rather than relying on
// the IDE's auto-generated prototypes
float read_water_level_cm();
float read_soil_moisture_percent();
float read_rainfall_intensity();
SensorReading read_sensors();

// ----------------------- sensor read helpers -----------------------

float read_water_level_cm() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // 30ms timeout ~ 5m max range, well beyond AJ-SR04M's spec
    long duration_us = pulseIn(ECHO_PIN, HIGH, 30000UL);
    if (duration_us == 0) {
        // no echo received - out of range or sensor fault. Return the
        // "empty" reading rather than 0, so this doesn't look like a
        // sudden flood to the DSP pipeline.
        return 0.0f;
    }

    float distance_cm = duration_us * 0.0343f / 2.0f;
    float water_level = SENSOR_HEIGHT_CM - distance_cm;
    if (water_level < 0.0f) water_level = 0.0f;
    return water_level;
}

float read_soil_moisture_percent() {
    int raw = analogRead(SOIL_PIN);
    float percent = (float)(raw - SOIL_DRY_RAW) / (float)(SOIL_WET_RAW - SOIL_DRY_RAW) * 100.0f;
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;
    return percent;
}

float read_rainfall_intensity() {
    int raw = analogRead(RAIN_PIN);
    float intensity = (float)(RAIN_DRY_RAW - raw) / (float)(RAIN_DRY_RAW - RAIN_WET_RAW) * 100.0f;
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 100.0f) intensity = 100.0f;
    return intensity;
}

SensorReading read_sensors() {
    SensorReading r;
    r.water_level   = read_water_level_cm();
    r.rainfall      = read_rainfall_intensity();
    r.pressure      = bme_ok ? (bme.readPressure() / 100.0f) : 1013.0f;  // hPa
    r.temperature   = bme_ok ? bme.readTemperature() : 25.0f;
    r.humidity      = bme_ok ? bme.readHumidity() : 50.0f;
    r.soil_moisture = read_soil_moisture_percent();
    r.wind_speed    = 0.0f;  // no anemometer in your parts list - unused by fusion anyway
    return r;
}

// ----------------------- setup / loop -----------------------

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("PRAKRITI Node 1 booting...");

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(SOIL_PIN, INPUT);
    pinMode(RAIN_PIN, INPUT);

    Wire.begin(BME_SDA, BME_SCL);
    bme_ok = bme.begin(BME_ADDRESS);
    if (!bme_ok) {
        Serial.println("BME280 not found at 0x76 - trying 0x77...");
        bme_ok = bme.begin(0x77);
    }
    Serial.println(bme_ok ? "BME280 OK" : "BME280 FAILED - check wiring/address, using placeholder values");

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    int state = radio.begin(LORA_FREQ_MHZ);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("LoRa init failed, code "); Serial.println(state);
    } else {
        radio.setBandwidth(LORA_BANDWIDTH);
        radio.setSpreadingFactor(LORA_SPREADING);
        radio.setCodingRate(LORA_CODING_RATE);
        radio.setSyncWord(LORA_SYNC_WORD);
        radio.setOutputPower(LORA_TX_POWER);
        Serial.println("LoRa OK");
    }

    sc_reset();
    fe_reset();
    ab_reset();
    ta_reset();
}

void loop() {
    SensorReading reading = read_sensors();

    float deviations[NUM_SENSORS] = {0};
    TemporalResult temporal[NUM_SENSORS];
    SensorId fusion_sensors[3] = {WATER_LEVEL, RAINFALL, PRESSURE};

    for (int i = 0; i < 3; i++) {
        SensorId id = fusion_sensors[i];
        float raw = get_reading(reading, id);

        ConditionedValue conditioned = sc_condition_signal(id, raw);
        Features features = fe_extract_features(id, conditioned.filtered);
        float deviation = ab_update_baseline(id, conditioned.filtered, hazard_active);
        temporal[id] = ta_analyze(id, features, deviation);
        deviations[id] = deviation;
    }

    FusionResult fusion = fuse_flood_risk(deviations, temporal);
    RiskState state = classify_risk(fusion.fused_score);
    hazard_active = (state != RISK_NORMAL);

    Serial.print("water_level=");   Serial.print(reading.water_level);
    Serial.print("cm  rainfall=");  Serial.print(reading.rainfall);
    Serial.print("  pressure=");    Serial.print(reading.pressure);
    Serial.print("hPa  temp=");     Serial.print(reading.temperature);
    Serial.print("C  humidity=");   Serial.print(reading.humidity);
    Serial.print("%  soil=");       Serial.print(reading.soil_moisture);
    Serial.print("%  fused_score="); Serial.print(fusion.fused_score);
    Serial.print("  state=");       Serial.println(risk_state_name(state));

    // ---- build and send the LoRa packet ----
    LoRaPacket packet;
    packet.node_id       = 1;
    packet.seq           = packet_seq++;
    packet.water_level   = reading.water_level;
    packet.rainfall      = reading.rainfall;
    packet.pressure      = reading.pressure;
    packet.temperature   = reading.temperature;
    packet.humidity       = reading.humidity;
    packet.soil_moisture  = reading.soil_moisture;
    packet.fused_score    = fusion.fused_score;
    packet.risk_state     = (uint8_t)state;

    int tx_state = radio.transmit((uint8_t*)&packet, sizeof(packet));
    if (tx_state == RADIOLIB_ERR_NONE) {
        Serial.println("LoRa packet sent");
    } else {
        Serial.print("LoRa send failed, code "); Serial.println(tx_state);
    }

    // adaptive sampling: fast while a hazard is developing, slow when calm
    delay(hazard_active ? 1000 : 5000);
}
