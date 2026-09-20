#pragma once
#include "SensorTypes.h"
#include "temporal_analysis.h"

enum RiskState { RISK_NORMAL, RISK_WATCH, RISK_WARNING, RISK_CRITICAL };

struct FusionResult {
    float fused_score;
    bool faulty[NUM_SENSORS];  // only WATER_LEVEL/RAINFALL/PRESSURE are ever set
};

FusionResult fuse_flood_risk(const float deviations[NUM_SENSORS], const TemporalResult temporal[NUM_SENSORS]);
RiskState classify_risk(float fused_score);
const char* risk_state_name(RiskState state);
