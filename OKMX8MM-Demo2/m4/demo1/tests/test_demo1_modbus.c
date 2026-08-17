#include <stdint.h>

#include "demo1_modbus.h"
#include "demo1_types.h"
#include "test_harness.h"

static int test_crc16_matches_modbus_read_request(void) {
    const uint8_t request_without_crc[] = {0x01U, 0x03U, 0x00U, 0x00U, 0x00U, 0x0AU};

    TEST_ASSERT_EQ_INT(0xCDC5, demo1_modbus_crc16(request_without_crc, sizeof(request_without_crc)));
    return 0;
}

static int test_builds_read_request_for_ten_registers(void) {
    uint8_t frame[8];
    const uint8_t expected[] = {0x01U, 0x03U, 0x00U, 0x00U, 0x00U, 0x0AU, 0xC5U, 0xCDU};

    TEST_ASSERT_EQ_INT(8, demo1_modbus_build_read_request(1U, 0U, 10U, frame, sizeof(frame)));
    for (uint32_t index = 0U; index < sizeof(expected); ++index) {
        TEST_ASSERT_EQ_INT(expected[index], frame[index]);
    }
    return 0;
}

static int test_parses_ten_register_response(void) {
    const uint8_t response[] = {
        0x01U, 0x03U, 0x14U,
        0x00U, 0x64U, 0x00U, 0x65U, 0x00U, 0x66U, 0x00U, 0x67U, 0x00U, 0x68U,
        0x00U, 0x69U, 0x00U, 0x6AU, 0x00U, 0x6BU, 0x00U, 0x6CU, 0x00U, 0x6DU,
        0x63U, 0xD1U
    };
    uint16_t registers[10];
    uint16_t count = 0U;

    TEST_ASSERT_EQ_INT(0, demo1_modbus_parse_read_response(1U, response, sizeof(response),
                                                           registers, 10U, &count));
    TEST_ASSERT_EQ_INT(10, count);
    TEST_ASSERT_EQ_INT(100, registers[0]);
    TEST_ASSERT_EQ_INT(109, registers[9]);
    return 0;
}

static int test_rejects_response_with_bad_crc(void) {
    const uint8_t response[] = {
        0x01U, 0x03U, 0x02U, 0x12U, 0x34U, 0x00U, 0x00U
    };
    uint16_t registers[1];
    uint16_t count = 99U;

    TEST_ASSERT_EQ_INT(-3, demo1_modbus_parse_read_response(1U, response, sizeof(response),
                                                            registers, 1U, &count));
    TEST_ASSERT_EQ_INT(0, count);
    return 0;
}

static int test_converts_registers_to_tick_sample(void) {
    const uint16_t registers[] = {100U, 101U, 102U, 103U, 104U, 105U, 106U, 107U, 108U, 109U};
    demo1_tick_sample_t sample;

    TEST_ASSERT_EQ_INT(0, demo1_modbus_registers_to_tick_sample(registers, 10U, &sample));
    TEST_ASSERT_EQ_INT(10, sample.analog_channel_count);
    TEST_ASSERT_EQ_INT(0x03FF, sample.valid_mask);
    TEST_ASSERT_EQ_INT(100, sample.analog[0]);
    TEST_ASSERT_EQ_INT(109, sample.analog[9]);
    TEST_ASSERT_EQ_INT(0, sample.analog[10]);
    return 0;
}

int main(void) {
    TEST_RUN(test_crc16_matches_modbus_read_request);
    TEST_RUN(test_builds_read_request_for_ten_registers);
    TEST_RUN(test_parses_ten_register_response);
    TEST_RUN(test_rejects_response_with_bad_crc);
    TEST_RUN(test_converts_registers_to_tick_sample);
    return 0;
}
