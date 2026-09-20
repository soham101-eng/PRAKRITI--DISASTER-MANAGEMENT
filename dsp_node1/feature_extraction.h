#pragma once
#include "SensorTypes.h"

#define FEATURE_WINDOW 10

struct Features {
    float mean;
    float rms;
    float variance;
    float rate_of_change;
    float trend;
    float latest;
};

void fe_reset();
Features fe_extract_features(SensorId id, float filtered_value);
