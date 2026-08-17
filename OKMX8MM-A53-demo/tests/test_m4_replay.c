#include "a53_demo.h"
#include "test_harness.h"

static int replay_source_returns_deterministic_m4_batches(void)
{
    a53_m4_source_t source;
    a53_m4_batch_t first;
    a53_m4_batch_t second;

    TEST_ASSERT_EQ_INT(0, a53_m4_replay_open(&source));
    TEST_ASSERT_EQ_INT(0, a53_m4_source_read(&source, &first));
    TEST_ASSERT_EQ_INT(0, a53_m4_source_read(&source, &second));
    a53_m4_source_close(&source);

    TEST_ASSERT_EQ_INT(0, first.sequence);
    TEST_ASSERT_EQ_INT(1, second.sequence);
    TEST_ASSERT_EQ_INT(2000, first.sample_rate_hz);
    TEST_ASSERT_EQ_INT(10, first.sample_count);
    TEST_ASSERT_EQ_INT(10, first.analog_channel_count);
    TEST_ASSERT_EQ_INT(0x03ff, first.aggregate_valid_mask);
    TEST_ASSERT_EQ_INT(1000, first.analog_samples[0][0]);
    TEST_ASSERT_EQ_INT(1009, first.analog_samples[0][9]);
    TEST_ASSERT_EQ_INT(1010, first.analog_samples[1][0]);
    TEST_ASSERT_EQ_INT(0x01, first.digital_states[0]);
    TEST_ASSERT_EQ_INT(0x00, first.digital_states[1]);
    TEST_ASSERT_EQ_INT(11, first.speed_pulse_delta);
    TEST_ASSERT_EQ_INT(50000, first.speed_period_us);

    return 0;
}

int main(void)
{
    TEST_RUN(replay_source_returns_deterministic_m4_batches);
    return 0;
}
