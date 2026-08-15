#include "demo1_engine.h"

#include <string.h>

static void demo1_reset_batch(demo1_batch_t *batch) {
    memset(batch, 0, sizeof(*batch));
    batch->sample_rate_hz = DEMO1_SAMPLE_RATE_HZ;
    batch->sample_count = DEMO1_BATCH_SAMPLE_COUNT;
    batch->analog_channel_count = DEMO1_ANALOG_CHANNEL_COUNT;
}

static int demo1_engine_read_mock_tick(void *context,
                                       uint64_t timestamp_us,
                                       demo1_tick_sample_t *out) {
    return demo1_mock_source_read_tick((demo1_mock_source_t *)context, timestamp_us, out);
}

int demo1_engine_init_with_source(demo1_engine_t *engine,
                                  void *source_context,
                                  demo1_sample_read_fn read_tick) {
    if (engine == NULL || source_context == NULL || read_tick == NULL) {
        return -1;
    }

    memset(engine, 0, sizeof(*engine));
    engine->source_context = source_context;
    engine->read_tick = read_tick;
    demo1_reset_batch(&engine->current_batch);
    demo1_reset_batch(&engine->ready_batch);
    return 0;
}

int demo1_engine_init(demo1_engine_t *engine, demo1_mock_source_t *source) {
    return demo1_engine_init_with_source(engine, source, demo1_engine_read_mock_tick);
}

bool demo1_engine_tick(demo1_engine_t *engine, uint64_t timestamp_us) {
    demo1_tick_sample_t sample;
    uint8_t index;
    uint8_t analog_channel_count;

    if (engine == NULL || engine->source_context == NULL || engine->read_tick == NULL) {
        return false;
    }
    if (engine->read_tick(engine->source_context, timestamp_us, &sample) != 0) {
        return false;
    }
    if (sample.analog_channel_count == 0U ||
        sample.analog_channel_count > DEMO1_ANALOG_CHANNEL_COUNT) {
        return false;
    }
    analog_channel_count = sample.analog_channel_count;

    index = engine->sample_index;
    if (index == 0U) {
        demo1_reset_batch(&engine->current_batch);
        engine->current_batch.analog_channel_count = analog_channel_count;
        engine->current_batch.sequence = engine->next_sequence;
        engine->current_batch.start_timestamp_us = timestamp_us;
    }

    engine->current_batch.aggregate_valid_mask |= sample.valid_mask;
    engine->current_batch.digital_states[index] = sample.digital_bits;
    for (uint32_t channel = 0U; channel < analog_channel_count; ++channel) {
        engine->current_batch.analog_samples[index][channel] = sample.analog[channel];
    }
    engine->current_batch.latest_hall_pulse_delta[0] = sample.hall_pulse_delta[0];
    engine->current_batch.latest_hall_pulse_delta[1] = sample.hall_pulse_delta[1];
    engine->current_batch.latest_hall_period_us[0] = sample.hall_period_us[0];
    engine->current_batch.latest_hall_period_us[1] = sample.hall_period_us[1];
    engine->current_batch.latest_oil_raw[0] = sample.oil_raw[0];
    engine->current_batch.latest_oil_raw[1] = sample.oil_raw[1];
    engine->current_batch.latest_oil_status[0] = sample.oil_status[0];
    engine->current_batch.latest_oil_status[1] = sample.oil_status[1];
    engine->current_batch.latest_oil_age_ms[0] = sample.oil_age_ms[0];
    engine->current_batch.latest_oil_age_ms[1] = sample.oil_age_ms[1];

    engine->sample_index = (uint8_t)(index + 1U);
    if (engine->sample_index < DEMO1_BATCH_SAMPLE_COUNT) {
        return false;
    }

    engine->ready_batch = engine->current_batch;
    engine->batch_ready = true;
    engine->sample_index = 0U;
    engine->next_sequence += 1U;
    return true;
}

int demo1_engine_consume_batch(demo1_engine_t *engine, demo1_batch_t *out) {
    if (engine == NULL || out == NULL) {
        return -1;
    }
    if (!engine->batch_ready) {
        return -2;
    }

    *out = engine->ready_batch;
    engine->batch_ready = false;
    return 0;
}
