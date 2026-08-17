#ifndef DEMO1_TYPES_H
#define DEMO1_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DEMO1_ANALOG_CHANNEL_COUNT = 14,
    DEMO1_BATCH_SAMPLE_COUNT = 10,
    DEMO1_SAMPLE_RATE_HZ = 2000,
    DEMO1_TICK_PERIOD_US = 500
};

typedef struct {
    uint8_t analog_channel_count;
    uint16_t valid_mask;
    int16_t analog[DEMO1_ANALOG_CHANNEL_COUNT];
    uint8_t digital_bits;
    uint16_t hall_pulse_delta[2];
    uint32_t hall_period_us[2];
    int32_t oil_raw[2];
    uint16_t oil_status[2];
    uint16_t oil_age_ms[2];
} demo1_tick_sample_t;

typedef struct {
    uint32_t sequence;
    uint64_t start_timestamp_us;
    uint32_t sample_rate_hz;
    uint8_t sample_count;
    uint8_t analog_channel_count;
    uint16_t aggregate_valid_mask;
    int16_t analog_samples[DEMO1_BATCH_SAMPLE_COUNT][DEMO1_ANALOG_CHANNEL_COUNT];
    uint8_t digital_states[DEMO1_BATCH_SAMPLE_COUNT];
    uint16_t latest_hall_pulse_delta[2];
    uint32_t latest_hall_period_us[2];
    int32_t latest_oil_raw[2];
    uint16_t latest_oil_status[2];
    uint16_t latest_oil_age_ms[2];
} demo1_batch_t;

#endif
