#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "demo1_modbus_source.h"
#include "test_harness.h"

typedef struct {
    uint8_t written[16];
    size_t written_length;
    uint8_t response[32];
    size_t response_length;
} fake_transport_t;

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
        transport->response_length == 0U ||
        capacity < transport->response_length) {
        return -1;
    }
    memcpy(data, transport->response, transport->response_length);
    *out_length = transport->response_length;
    return 0;
}

static int test_reads_modbus_registers_into_tick_sample(void) {
    fake_transport_t transport = {
        {0U},
        0U,
        {
            0x01U, 0x03U, 0x14U,
            0x00U, 0x64U, 0x00U, 0x65U, 0x00U, 0x66U, 0x00U, 0x67U, 0x00U, 0x68U,
            0x00U, 0x69U, 0x00U, 0x6AU, 0x00U, 0x6BU, 0x00U, 0x6CU, 0x00U, 0x6DU,
            0x63U, 0xD1U
        },
        25U
    };
    demo1_modbus_source_t source;
    demo1_tick_sample_t sample;

    TEST_ASSERT_EQ_INT(0, demo1_modbus_source_init(&source, &transport, fake_write, fake_read,
                                                   1U, 0U, 10U));
    TEST_ASSERT_EQ_INT(0, demo1_modbus_source_read_tick(&source, 0ULL, &sample));

    TEST_ASSERT_EQ_INT(8, transport.written_length);
    TEST_ASSERT_EQ_INT(0x01, transport.written[0]);
    TEST_ASSERT_EQ_INT(0x03, transport.written[1]);
    TEST_ASSERT_EQ_INT(0x0A, transport.written[5]);
    TEST_ASSERT_EQ_INT(10, sample.analog_channel_count);
    TEST_ASSERT_EQ_INT(0x03FF, sample.valid_mask);
    TEST_ASSERT_EQ_INT(100, sample.analog[0]);
    TEST_ASSERT_EQ_INT(109, sample.analog[9]);
    return 0;
}

static int test_rejects_transport_read_failure(void) {
    fake_transport_t transport = {{0U}, 0U, {0U}, 0U};
    demo1_modbus_source_t source;
    demo1_tick_sample_t sample;

    TEST_ASSERT_EQ_INT(0, demo1_modbus_source_init(&source, &transport, fake_write, fake_read,
                                                   1U, 0U, 10U));
    TEST_ASSERT_EQ_INT(-3, demo1_modbus_source_read_tick(&source, 0ULL, &sample));
    return 0;
}

int main(void) {
    TEST_RUN(test_reads_modbus_registers_into_tick_sample);
    TEST_RUN(test_rejects_transport_read_failure);
    return 0;
}
