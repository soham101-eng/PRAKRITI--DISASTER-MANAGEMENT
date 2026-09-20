#pragma once
#include "SensorTypes.h"

void ab_reset();
float ab_update_baseline(SensorId id, float value, bool hazard_active);
