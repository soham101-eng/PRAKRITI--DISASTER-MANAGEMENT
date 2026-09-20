#pragma once
#include <stdint.h>

// Sent by Node 1, received by Node 4. Keep this file IDENTICAL on both
// boards - if the struct layout differs, Node 4 will decode garbage.
struct __attribute__((packed)) LoRaPacket {
    uint8_t  node_id;        // 1 = Node 1
    uint32_t seq;             // increments every send, lets Node 4 spot drops
    float    water_level;     // cm
    float    rainfall;        // 0-100 relative intensity
    float    pressure;        // hPa
    float    temperature;     // C
    float    humidity;        // %
    float    soil_moisture;   // %
    float    fused_score;     // DSP pipeline output
    uint8_t  risk_state;      // 0=NORMAL 1=WATCH 2=WARNING 3=CRITICAL
};
