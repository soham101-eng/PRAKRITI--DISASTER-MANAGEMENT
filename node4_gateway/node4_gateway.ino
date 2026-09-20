// =======================================================================
// PRAKRITI - NODE 4 (Gateway, test build) - ESP32-S3-N16R8
// Receives Node 1's LoRa packet and prints it to Serial Monitor.
// Per your wiring guide: SIM800L is skipped for now - this Serial
// output is your stand-in "backend" until the real dashboard exists.
// =======================================================================

#include <SPI.h>
#include <RadioLib.h>
#include "LoRaPacket.h"

// ----------------------- PIN MAP (same LoRa wiring as Node 1) -----------------------
#define LORA_SCK    12
#define LORA_MISO   13
#define LORA_MOSI   11
#define LORA_NSS    10
#define LORA_RESET  9
#define LORA_DIO0   8

// ----------------------- LoRa settings - MUST MATCH NODE 1 EXACTLY -----------------------
#define LORA_FREQ_MHZ     433.0
#define LORA_BANDWIDTH    125.0
#define LORA_SPREADING    9
#define LORA_CODING_RATE  7
#define LORA_SYNC_WORD    0x12

SX1278 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RESET, RADIOLIB_NC);

const char* risk_name(uint8_t state);

const char* risk_name(uint8_t state) {
    switch (state) {
        case 0: return "NORMAL";
        case 1: return "WATCH";
        case 2: return "WARNING";
        default: return "CRITICAL";
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("PRAKRITI Node 4 (Gateway) booting...");

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    int state = radio.begin(LORA_FREQ_MHZ);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("LoRa init failed, code "); Serial.println(state);
        return;
    }
    radio.setBandwidth(LORA_BANDWIDTH);
    radio.setSpreadingFactor(LORA_SPREADING);
    radio.setCodingRate(LORA_CODING_RATE);
    radio.setSyncWord(LORA_SYNC_WORD);
    Serial.println("LoRa OK - listening for Node 1...");
}

void loop() {
    LoRaPacket packet;
    int state = radio.receive((uint8_t*)&packet, sizeof(packet));

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("---- Packet received ----");
        Serial.print("From node:      "); Serial.println(packet.node_id);
        Serial.print("Sequence:       "); Serial.println(packet.seq);
        Serial.print("Water level:    "); Serial.print(packet.water_level); Serial.println(" cm");
        Serial.print("Rainfall:       "); Serial.println(packet.rainfall);
        Serial.print("Pressure:       "); Serial.print(packet.pressure); Serial.println(" hPa");
        Serial.print("Temperature:    "); Serial.print(packet.temperature); Serial.println(" C");
        Serial.print("Humidity:       "); Serial.print(packet.humidity); Serial.println(" %");
        Serial.print("Soil moisture:  "); Serial.print(packet.soil_moisture); Serial.println(" %");
        Serial.print("Fused score:    "); Serial.println(packet.fused_score);
        Serial.print("Risk state:     "); Serial.println(risk_name(packet.risk_state));
        Serial.print("RSSI:           "); Serial.print(radio.getRSSI()); Serial.println(" dBm");
        Serial.println("--------------------------");
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
        // normal - just means nothing arrived in this listen window
    } else {
        Serial.print("Receive error, code "); Serial.println(state);
    }
}
