#include "signal_conditioning.h"

// --- Calibration: real_value = raw * scale + offset ---
// Update these tomorrow once you calibrate each sensor against a known
// reference (e.g. dip a ruler next to the water level sensor).
static const float CAL_SCALE[NUM_SENSORS]  = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
static const float CAL_OFFSET[NUM_SENSORS] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

// --- Normalization ranges, in physical units (adjust to your datasheets) ---
static const float RANGE_MIN[NUM_SENSORS] = {0,   0,   950, 0,  0,   0,   0};
static const float RANGE_MAX[NUM_SENSORS] = {500, 100, 1050, 50, 100, 100, 150};

// --- Moving-average filter: one small ring buffer per sensor ---
static float filter_buf[NUM_SENSORS][FILTER_WINDOW];
static int   filter_idx[NUM_SENSORS];
static int   filter_count[NUM_SENSORS];

void sc_reset() {
    for (int s = 0; s < NUM_SENSORS; s++) {
        filter_idx[s] = 0;
        filter_count[s] = 0;
        for (int i = 0; i < FILTER_WINDOW; i++) filter_buf[s][i] = 0.0f;
    }
}

static float sc_calibrate(SensorId id, float raw) {
    return raw * CAL_SCALE[id] + CAL_OFFSET[id];
}

static float sc_filter(SensorId id, float calibrated) {
    filter_buf[id][filter_idx[id]] = calibrated;
    filter_idx[id] = (filter_idx[id] + 1) % FILTER_WINDOW;
    if (filter_count[id] < FILTER_WINDOW) filter_count[id]++;

    float sum = 0.0f;
    for (int i = 0; i < filter_count[id]; i++) sum += filter_buf[id][i];
    return sum / filter_count[id];
}

static float sc_normalize(SensorId id, float value) {
    float lo = RANGE_MIN[id], hi = RANGE_MAX[id];
    float n = (value - lo) / (hi - lo);
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return n;
}

ConditionedValue sc_condition_signal(SensorId id, float raw_value) {
    float calibrated = sc_calibrate(id, raw_value);
    float filtered = sc_filter(id, calibrated);
    float normalized = sc_normalize(id, filtered);
    return ConditionedValue{filtered, normalized};
}
