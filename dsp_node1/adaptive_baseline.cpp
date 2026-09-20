#include "adaptive_baseline.h"
#include <math.h>

#define ALPHA_NORMAL 0.02f
#define ALPHA_HAZARD 0.002f

static float baseline_mean[NUM_SENSORS];
static float baseline_var[NUM_SENSORS];
static bool  baseline_init[NUM_SENSORS];

void ab_reset() {
    for (int s = 0; s < NUM_SENSORS; s++) {
        baseline_init[s] = false;
        baseline_mean[s] = 0.0f;
        baseline_var[s] = 1e-6f;
    }
}

float ab_update_baseline(SensorId id, float value, bool hazard_active) {
    float alpha = hazard_active ? ALPHA_HAZARD : ALPHA_NORMAL;

    if (!baseline_init[id]) {
        baseline_mean[id] = value;
        baseline_var[id] = 1e-6f;
        baseline_init[id] = true;
        return 0.0f;
    }

    float prev_mean = baseline_mean[id];
    baseline_mean[id] = (1 - alpha) * prev_mean + alpha * value;

    float diff = value - prev_mean;
    baseline_var[id] = (1 - alpha) * baseline_var[id] + alpha * diff * diff;

    float std_dev = sqrtf(baseline_var[id]);
    if (std_dev < 1e-6f) std_dev = 1e-6f;

    return diff / std_dev;
}
