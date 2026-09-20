#include "feature_extraction.h"
#include <math.h>

static float hist[NUM_SENSORS][FEATURE_WINDOW];
static int   hist_idx[NUM_SENSORS];
static int   hist_count[NUM_SENSORS];

void fe_reset() {
    for (int s = 0; s < NUM_SENSORS; s++) {
        hist_idx[s] = 0;
        hist_count[s] = 0;
    }
}

Features fe_extract_features(SensorId id, float filtered_value) {
    hist[id][hist_idx[id]] = filtered_value;
    hist_idx[id] = (hist_idx[id] + 1) % FEATURE_WINDOW;
    if (hist_count[id] < FEATURE_WINDOW) hist_count[id]++;

    int n = hist_count[id];
    // walk the ring buffer oldest -> newest
    int start = (hist_idx[id] - n + FEATURE_WINDOW) % FEATURE_WINDOW;

    float values[FEATURE_WINDOW];
    float sum = 0.0f, sum_sq = 0.0f;
    for (int i = 0; i < n; i++) {
        float v = hist[id][(start + i) % FEATURE_WINDOW];
        values[i] = v;
        sum += v;
        sum_sq += v * v;
    }
    float mean = sum / n;
    float rms = sqrtf(sum_sq / n);

    float var_sum = 0.0f;
    for (int i = 0; i < n; i++) var_sum += (values[i] - mean) * (values[i] - mean);
    float variance = var_sum / n;

    float rate_of_change = (n >= 2) ? (values[n - 1] - values[n - 2]) : 0.0f;

    float trend = 0.0f;
    if (n >= 2) {
        float x_mean = (n - 1) / 2.0f;
        float num = 0.0f, den = 0.0f;
        for (int i = 0; i < n; i++) {
            num += (i - x_mean) * (values[i] - mean);
            den += (i - x_mean) * (i - x_mean);
        }
        if (den < 1e-9f) den = 1e-9f;
        trend = num / den;
    }

    Features f;
    f.mean = mean;
    f.rms = rms;
    f.variance = variance;
    f.rate_of_change = rate_of_change;
    f.trend = trend;
    f.latest = values[n - 1];
    return f;
}
