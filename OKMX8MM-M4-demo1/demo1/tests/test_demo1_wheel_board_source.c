#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "demo1_modbus.h"
#include "demo1_wheel_board_source.h"
#include "test_harness.h"

typedef struct {
    uint8_t written[16];
    size_t written_length;
    uint8_t response[32];
    size_t response_length;
} fake_transport_t;

static void append_crc(uint8_t *frame, size_t payload_length) {
    uint16_t crc = demo1_modbus_crc16(frame, payload_length);
    frame[payload_length] = (uint8_t)(crc & 0xFFU);
    frame[payload_length + 1U] = (uint8_t)(crc >> 8U);
}

static int fake_write(void *context, const uint8_t *data, size_t length) {
    fake_transport_t *transport = (fake_transport_t *)context;

    if (transport == NULL || data == NULL || length > sizeof(transport->written)) {
        return -1;
    }
    memcpy(transport->written, data, length);
    transport->written_length = length;
    return 0;
}

static int fake_read(void *context, uint8_t *data, size_t capacity, size_t *out_length) {
    fake_transport_t *transport = (fake_transport_t *)context;

    if (transport == NULL || data == NULL || out_length == NULL ||
        capacity < transport->response_length) {
        return -1;
    }
    memcpy(data, transport->response, transport->response_length);
    *out_length = transport->response_length;
    return 0;
}

static int test_reads_wheel_board_layout_into_tick_sample(void) {
    fake_transport_t transport = {{0U}, 0U, {0U}, 0U};
    demo1_wheel_board_source_t source;
    demo1_tick_sample_t sample;
    uint8_t response[] = {
        0x01U, 0x03U, 0x14U,
        0x04U, 0xB0U, 0x04U, 0xB1U, 0x04U, 0xB2U, 0x04U, 0xB3U,
        0x04U, 0xB4U, 0x04U, 0xB5U, 0x04U, 0xB6U, 0x04U, 0xB7U,
        0x00U, 0x03U, 0x03U, 0x52U,
        0x00U, 0x00U
    };

    append_crc(response, sizeof(response) - 2U);
    memcpy(transport.response, response, sizeof(response));
    transport.response_length = sizeof(response);

    TEST_ASSERT_EQ_INT(0, demo1_wheel_board_source_init(&source,
                                                        &transport,
                                                        fake_write,
                                                        fake_read,
                                                        1U,
                                                        0U));
    TEST_ASSERT_EQ_INT(0, demo1_wheel_board_source_read_tick(&source, 500ULL, &sample));

    TEST_ASSERT_EQ_INT(8, transport.written_length);
    TEST_ASSERT_EQ_INT(0x01, transport.written[0]);
    TEST_ASSERT_EQ_INT(0x03, transport.written[1]);
    TEST_ASSERT_EQ_INT(10, transport.written[5]);
    TEST_ASSERT_EQ_INT(8, sample.analog_channel_count);
    TEST_ASSERT_EQ_INT(0x00FF, sample.valid_mask);
    TEST_ASSERT_EQ_INT(1200, sample.analog[0]);
    TEST_ASSERT_EQ_INT(1207, sample.analog[7]);
    TEST_ASSERT_EQ_INT(0x03, sample.digital_bits);
    TEST_ASSERT_EQ_INT(850, sample.hall_pulse_delta[0]);
    return 0;
}

static int test_rejects_bad_wheel_board_response(void) {
    fake_transport_t transport = {{0U}, 0U, {0x01U, 0x03U, 0x00U, 0x00U, 0x00U}, 5U};
    demo1_wheel_board_source_t source;
    demo1_tick_sample_t sample;

    TEST_ASSERT_EQ_INT(0, demo1_wheel_board_source_init(&source,
                                                        &transport,
                                                        fake_write,
                                                        fake_read,
                                                        1U,
                                                        0U));
    TEST_ASSERT_EQ_INT(-4, demo1_wheel_board_source_read_tick(&source, 0ULL, &sample));
    return 0;
}

int main(void) {
    TEST_RUN(test_reads_wheel_board_layout_into_tick_sample);
    TEST_RUN(test_rejects_bad_wheel_board_response);
    return 0;
}
