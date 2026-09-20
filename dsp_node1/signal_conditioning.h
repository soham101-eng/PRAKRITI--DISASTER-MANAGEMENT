#pragma once
#include "SensorTypes.h"

#define FILTER_WINDOW 5

struct ConditionedValue {
    float filtered;
    float normalized;
};

void sc_reset();
ConditionedValue sc_condition_signal(SensorId id, float raw_value);
