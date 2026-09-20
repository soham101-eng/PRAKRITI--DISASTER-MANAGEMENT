#pragma once

// One entry per physical sensor on Node 1. Used to index every fixed-size
// array in the pipeline instead of using a hash map - cheaper and more
// predictable on an MCU.
enum SensorId {
    WATER_LEVEL = 0,
    RAINFALL,
    PRESSURE,
    TEMPERATURE,
    HUMIDITY,
    SOIL_MOISTURE,
    WIND_SPEED,
    NUM_SENSORS
};

struct SensorReading {
    float water_level;
    float rainfall;
    float pressure;
    float temperature;
    float humidity;
    float soil_moisture;
    float wind_speed;
};

// Lets the pipeline loop over sensors generically instead of hardcoding
// which struct field goes with which SensorId everywhere.
inline float get_reading(const SensorReading& r, SensorId id) {
    switch (id) {
        case WATER_LEVEL:   return r.water_level;
        case RAINFALL:      return r.rainfall;
        case PRESSURE:      return r.pressure;
        case TEMPERATURE:   return r.temperature;
        case HUMIDITY:      return r.humidity;
        case SOIL_MOISTURE: return r.soil_moisture;
        case WIND_SPEED:    return r.wind_speed;
        default:            return 0.0f;
    }
}
