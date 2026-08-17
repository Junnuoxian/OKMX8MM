#include <stdint.h>
#include <string.h>

#include "demo1_mock_source.h"
#include "test_harness.h"

static int test_initialize_rejects_invalid_arguments(void) {
    demo1_mock_source_t source;
    demo1_tick_sample_t sample;

    TEST_ASSERT_EQ_INT(-1, demo1_mock_source_init(NULL, 20260804U));
    TEST_ASSERT_EQ_INT(-1, demo1_mock_source_read_tick(NULL, 0U, &sample));
    TEST_ASSERT_EQ_INT(-1, demo1_mock_source_read_tick(&source, 0U, NULL));
    return 0;
}

static int test_seed_is_deterministic_for_first_tick(void) {
    static const int16_t expected[DEMO1_ANALOG_CHANNEL_COUNT] = {
        98, 199, 300, 401, 502, 603,
        1004, 1105, 1195, 1296, 1397, 1498, 1599, 1700
    };
    demo1_mock_source_t source;
    demo1_tick_sample_t sample;

    TEST_ASSERT_EQ_INT(0, demo1_mock_source_init(&source, 20260804U));
    TEST_ASSERT_EQ_INT(0, demo1_mock_source_read_tick(&source, 0U, &sample));
    TEST_ASSERT_EQ_INT(DEMO1_ANALOG_CHANNEL_COUNT, sample.analog_channel_count);
    TEST_ASSERT_EQ_INT(0x3FFF, sample.valid_mask);
    TEST_ASSERT_EQ_INT(0, sample.digital_bits);
    TEST_ASSERT_EQ_INT(4, sample.hall_pulse_delta[0]);
    TEST_ASSERT_EQ_INT(7, sample.hall_pulse_delta[1]);
    TEST_ASSERT_EQ_INT(5000, sample.oil_raw[0]);
    TEST_ASSERT_EQ_INT(5030, sample.oil_raw[1]);
    TEST_ASSERT_TRUE(memcmp(expected, sample.analog, sizeof(expected)) == 0);
    return 0;
}

static int test_equal_seeds_produce_equal_sequences(void) {
    demo1_mock_source_t first;
    demo1_mock_source_t second;
    demo1_tick_sample_t first_sample;
    demo1_tick_sample_t second_sample;

    TEST_ASSERT_EQ_INT(0, demo1_mock_source_init(&first, 99U));
    TEST_ASSERT_EQ_INT(0, demo1_mock_source_init(&second, 99U));
    TEST_ASSERT_EQ_INT(0, demo1_mock_source_read_tick(&first, 1500U, &first_sample));
    TEST_ASSERT_EQ_INT(0, demo1_mock_source_read_tick(&second, 1500U, &second_sample));
    TEST_ASSERT_TRUE(memcmp(&first_sample, &second_sample, sizeof(first_sample)) == 0);
    return 0;
}

int main(void) {
    TEST_RUN(test_initialize_rejects_invalid_arguments);
    TEST_RUN(test_seed_is_deterministic_for_first_tick);
    TEST_RUN(test_equal_seeds_produce_equal_sequences);
    return 0;
}
