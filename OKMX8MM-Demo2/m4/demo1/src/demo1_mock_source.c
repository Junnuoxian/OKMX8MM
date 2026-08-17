#include "demo1_mock_source.h"

#include <string.h>

static const int16_t k_demo1_analog_bases[DEMO1_ANALOG_CHANNEL_COUNT] = {
    100, 200, 300, 400, 500, 600,
    1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700
};

int demo1_mock_source_init(demo1_mock_source_t *source, uint32_t seed) {
    if (source == NULL) {
        return -1;
    }
    source->seed = seed;
    source->tick_count = 0U;
    source->initialized = 1;
    return 0;
}

int demo1_mock_source_read_tick(demo1_mock_source_t *source,
                                uint64_t timestamp_us,
                                demo1_tick_sample_t *out) {
    uint32_t seed_mod;
    uint32_t tick;

    (void)timestamp_us;

    if (source == NULL || out == NULL || source->initialized == 0) {
        return -1;
    }

    memset(out, 0, sizeof(*out));
    seed_mod = source->seed % 11U;
    tick = source->tick_count;

    out->analog_channel_count = DEMO1_ANALOG_CHANNEL_COUNT;
    out->valid_mask = 0x3FFFU;
    for (uint32_t channel = 0U; channel < DEMO1_ANALOG_CHANNEL_COUNT; ++channel) {
        int16_t offset = (int16_t)(((seed_mod + (tick * 2U) + channel) % 11U) - 5);
        out->analog[channel] = (int16_t)(k_demo1_analog_bases[channel] + offset);
    }

    if (tick >= 4U) {
        out->digital_bits |= 0x01U;
    }
    if (tick >= 7U) {
        out->digital_bits |= 0x02U;
    }

    out->hall_pulse_delta[0] = (uint16_t)(4U + tick);
    out->hall_pulse_delta[1] = (uint16_t)(7U + tick);
    out->hall_period_us[0] = 2500U + (tick * 10U);
    out->hall_period_us[1] = 3200U + (tick * 10U);

    out->oil_raw[0] = 5000 + (int32_t)(tick * 20U);
    out->oil_raw[1] = 5030 + (int32_t)(tick * 20U);
    out->oil_status[0] = 0U;
    out->oil_status[1] = 0U;
    out->oil_age_ms[0] = 0U;
    out->oil_age_ms[1] = 0U;

    source->tick_count = tick + 1U;
    return 0;
}
