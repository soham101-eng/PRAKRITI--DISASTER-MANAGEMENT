#include "temporal_analysis.h"
#include <math.h>

static float prev_rate[NUM_SENSORS];
static int   persist_counter[NUM_SENSORS];

void ta_reset() {
    for (int s = 0; s < NUM_SENSORS; s++) {
        prev_rate[s] = 0.0f;
        persist_counter[s] = 0;
    }
}

TemporalResult ta_analyze(SensorId id, const Features& features, float deviation, float persistence_threshold) {
    float acceleration = features.rate_of_change - prev_rate[id];
    prev_rate[id] = features.rate_of_change;

    if (fabsf(deviation) > persistence_threshold) {
        persist_counter[id]++;
    } else {
        persist_counter[id] = 0;
    }

    TemporalResult r;
    r.value = features.latest;
    r.trend = features.trend;
    r.rate = features.rate_of_change;
    r.acceleration = acceleration;
    r.persistence = persist_counter[id];
    return r;
}
