#pragma once
#include "SensorTypes.h"
#include "feature_extraction.h"

struct TemporalResult {
    float value;
    float trend;
    float rate;
    float acceleration;
    int persistence;
};

void ta_reset();
TemporalResult ta_analyze(SensorId id, const Features& features, float deviation, float persistence_threshold = 1.5f);
