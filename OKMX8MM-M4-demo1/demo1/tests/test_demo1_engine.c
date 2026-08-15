#include <stdint.h>
#include <string.h>

#include "demo1_console.h"
#include "demo1_engine.h"
#include "demo1_mock_source.h"
#include "test_harness.h"

typedef struct {
    uint32_t calls;
} fake_source_t;

static int fake_source_read_tick(void *context, uint64_t timestamp_us, demo1_tick_sample_t *out) {
    fake_source_t *source = (fake_source_t *)context;

    (void)timestamp_us;
    if (source == NULL || out == NULL) {
        return -1;
    }

    memset(out, 0, sizeof(*out));
    out->analog_channel_count = 10U;
    out->valid_mask = 0x03FFU;
    for (uint32_t channel = 0U; channel < 10U; ++channel) {
        out->analog[channel] = (int16_t)(1000 + (int32_t)source->calls + (int32_t)channel);
    }
    source->calls++;
    return 0;
}

static int test_engine_accepts_generic_sample_source(void) {
    fake_source_t source = {0U};
    demo1_engine_t engine;
    demo1_batch_t batch;

    TEST_ASSERT_EQ_INT(0, demo1_engine_init_with_source(&engine, &source, fake_source_read_tick));
    for (uint32_t tick = 0U; tick < 10U; ++tick) {
        (void)demo1_engine_tick(&engine, (uint64_t)tick * 500ULL);
    }

    TEST_ASSERT_EQ_INT(0, demo1_engine_consume_batch(&engine, &batch));
    TEST_ASSERT_EQ_INT(10, batch.analog_channel_count);
    TEST_ASSERT_EQ_INT(0x03FF, batch.aggregate_valid_mask);
    TEST_ASSERT_EQ_INT(1000, batch.analog_samples[0][0]);
    TEST_ASSERT_EQ_INT(1018, batch.analog_samples[9][9]);
    return 0;
}

static int test_engine_emits_batch_after_ten_ticks(void) {
    demo1_mock_source_t source;
    demo1_engine_t engine;
    demo1_batch_t batch;
    char line[160];

    TEST_ASSERT_EQ_INT(0, demo1_mock_source_init(&source, 20260804U));
    TEST_ASSERT_EQ_INT(0, demo1_engine_init(&engine, &source));
    for (uint32_t tick = 0U; tick < 9U; ++tick) {
        TEST_ASSERT_TRUE(!demo1_engine_tick(&engine, (uint64_t)tick * 500ULL));
    }
    TEST_ASSERT_TRUE(demo1_engine_tick(&engine, 4500ULL));
    TEST_ASSERT_EQ_INT(0, demo1_engine_consume_batch(&engine, &batch));
    TEST_ASSERT_EQ_INT(0, batch.sequence);
    TEST_ASSERT_EQ_INT(0, batch.start_timestamp_us);
    TEST_ASSERT_EQ_INT(2000, batch.sample_rate_hz);
    TEST_ASSERT_EQ_INT(10, batch.sample_count);
    TEST_ASSERT_EQ_INT(DEMO1_ANALOG_CHANNEL_COUNT, batch.analog_channel_count);
    TEST_ASSERT_EQ_INT(0x3FFF, batch.aggregate_valid_mask);
    TEST_ASSERT_EQ_INT(98, batch.analog_samples[0][0]);
    TEST_ASSERT_EQ_INT(599, batch.analog_samples[9][5]);
    TEST_ASSERT_EQ_INT(1, batch.digital_states[4]);
    TEST_ASSERT_EQ_INT(3, batch.digital_states[7]);
    TEST_ASSERT_EQ_INT(13, batch.latest_hall_pulse_delta[0]);
    TEST_ASSERT_EQ_INT(16, batch.latest_hall_pulse_delta[1]);
    TEST_ASSERT_EQ_INT(5180, batch.latest_oil_raw[0]);
    TEST_ASSERT_EQ_INT(5210, batch.latest_oil_raw[1]);
    TEST_ASSERT_TRUE(demo1_format_batch_status(&batch, line, sizeof(line)) > 0U);
    TEST_ASSERT_TRUE(strstr(line, "seq=0") != NULL);
    TEST_ASSERT_TRUE(strstr(line, "ticks=10") != NULL);
    return 0;
}

static int test_consume_without_ready_batch_fails(void) {
    demo1_mock_source_t source;
    demo1_engine_t engine;
    demo1_batch_t batch;

    TEST_ASSERT_EQ_INT(0, demo1_mock_source_init(&source, 11U));
    TEST_ASSERT_EQ_INT(0, demo1_engine_init(&engine, &source));
    TEST_ASSERT_EQ_INT(-2, demo1_engine_consume_batch(&engine, &batch));
    return 0;
}

int main(void) {
    TEST_RUN(test_engine_accepts_generic_sample_source);
    TEST_RUN(test_engine_emits_batch_after_ten_ticks);
    TEST_RUN(test_consume_without_ready_batch_fails);
    return 0;
}
