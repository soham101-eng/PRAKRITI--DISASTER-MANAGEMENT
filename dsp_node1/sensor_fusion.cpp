#include "sensor_fusion.h"
#include <math.h>

#define FAULT_Z_THRESHOLD 6.0f

FusionResult fuse_flood_risk(const float deviations[NUM_SENSORS], const TemporalResult temporal[NUM_SENSORS]) {
    // only these 3 sensors feed flood-risk fusion, per the architecture doc
    struct { SensorId id; float weight; bool invert; } sensors[3] = {
        {WATER_LEVEL, 0.5f, false},
        {RAINFALL,    0.3f, false},
        {PRESSURE,    0.2f, true},   // a pressure DROP is the precursor signal
    };

    FusionResult result;
    for (int i = 0; i < NUM_SENSORS; i++) result.faulty[i] = false;

    float weighted_sum = 0.0f, weight_total = 0.0f, persistence_sum = 0.0f;

    for (int i = 0; i < 3; i++) {
        SensorId id = sensors[i].id;
        float dev = deviations[id];
        float weight = sensors[i].weight;

        if (fabsf(dev) > FAULT_Z_THRESHOLD) {
            weight *= 0.1f;             // down-weight, don't let it dominate
            result.faulty[id] = true;
        }

        float contrib = sensors[i].invert ? -dev : dev;
        weighted_sum += weight * contrib;
        weight_total += weight;
        persistence_sum += temporal[id].persistence;
    }

    result.fused_score = (weight_total > 0) ? (weighted_sum / weight_total) : 0.0f;
    result.fused_score += 0.05f * (persistence_sum / 3.0f);
    return result;
}

RiskState classify_risk(float fused_score) {
    if (fused_score < 1.5f) return RISK_NORMAL;
    if (fused_score < 3.0f) return RISK_WATCH;
    if (fused_score < 5.0f) return RISK_WARNING;
    return RISK_CRITICAL;
}

const char* risk_state_name(RiskState state) {
    switch (state) {
        case RISK_NORMAL:  return "NORMAL";
        case RISK_WATCH:   return "WATCH";
        case RISK_WARNING: return "WARNING";
        default:            return "CRITICAL";
    }
}
