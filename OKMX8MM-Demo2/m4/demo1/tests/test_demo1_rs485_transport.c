#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "demo1_modbus.h"
#include "demo1_rs485_transport.h"
#include "test_harness.h"

typedef struct {
    uint8_t written[16];
    size_t written_length;
    uint8_t rx[32];
    size_t rx_length;
    size_t rx_index;
    uint64_t now_us;
} fake_port_t;

static void append_crc(uint8_t *frame, size_t payload_length) {
    uint16_t crc = demo1_modbus_crc16(frame, payload_length);
    frame[payload_length] = (uint8_t)(crc & 0xFFU);
    frame[payload_length + 1U] = (uint8_t)(crc >> 8U);
}

static int fake_write_bytes(void *context, const uint8_t *data, size_t length) {
    fake_port_t *port = (fake_port_t *)context;

    if (port == NULL || data == NULL || length > sizeof(port->written)) {
        return -1;
    }
    memcpy(port->written, data, length);
    port->written_length = length;
    return 0;
}

static int fake_read_byte(void *context, uint8_t *out_byte) {
    fake_port_t *port = (fake_port_t *)context;

    if (port == NULL || out_byte == NULL) {
        return -1;
    }
    if (port->rx_index >= port->rx_length) {
        port->now_us += 100U;
        return 0;
    }
    *out_byte = port->rx[port->rx_index++];
    port->now_us += 10U;
    return 1;
}

static uint64_t fake_now_us(void *context) {
    fake_port_t *port = (fake_port_t *)context;
    return port == NULL ? 0ULL : port->now_us;
}

static int test_writes_bytes_to_rs485_port(void) {
    fake_port_t port = {{0U}, 0U, {0U}, 0U, 0U, 0ULL};
    demo1_rs485_transport_t transport;
    const uint8_t request[] = {0x01U, 0x03U, 0x00U, 0x00U};

    TEST_ASSERT_EQ_INT(0, demo1_rs485_transport_init(&transport,
                                                     &port,
                                                     fake_write_bytes,
                                                     fake_read_byte,
                                                     fake_now_us,
                                                     1000U));
    TEST_ASSERT_EQ_INT(0, demo1_rs485_transport_write(&transport, request, sizeof(request)));

    TEST_ASSERT_EQ_INT(sizeof(request), port.written_length);
    TEST_ASSERT_TRUE(memcmp(request, port.written, sizeof(request)) == 0);
    return 0;
}

static int test_reads_complete_modbus_frame(void) {
    fake_port_t port = {{0U}, 0U, {0U}, 0U, 0U, 0ULL};
    demo1_rs485_transport_t transport;
    uint8_t out[32];
    size_t out_length = 0U;
    uint8_t response[] = {
        0x01U, 0x03U, 0x04U,
        0x00U, 0x64U, 0x00U, 0x65U,
        0x00U, 0x00U
    };

    append_crc(response, sizeof(response) - 2U);
    memcpy(port.rx, response, sizeof(response));
    port.rx_length = sizeof(response);

    TEST_ASSERT_EQ_INT(0, demo1_rs485_transport_init(&transport,
                                                     &port,
                                                     fake_write_bytes,
                                                     fake_read_byte,
                                                     fake_now_us,
                                                     1000U));
    TEST_ASSERT_EQ_INT(0, demo1_rs485_transport_read(&transport,
                                                    out,
                                                    sizeof(out),
                                                    &out_length));

    TEST_ASSERT_EQ_INT(sizeof(response), out_length);
    TEST_ASSERT_TRUE(memcmp(response, out, sizeof(response)) == 0);
    return 0;
}

static int test_returns_timeout_when_no_response_arrives(void) {
    fake_port_t port = {{0U}, 0U, {0U}, 0U, 0U, 0ULL};
    demo1_rs485_transport_t transport;
    uint8_t out[8];
    size_t out_length = 123U;

    TEST_ASSERT_EQ_INT(0, demo1_rs485_transport_init(&transport,
                                                     &port,
                                                     fake_write_bytes,
                                                     fake_read_byte,
                                                     fake_now_us,
                                                     500U));
    TEST_ASSERT_EQ_INT(-3, demo1_rs485_transport_read(&transport,
                                                     out,
                                                     sizeof(out),
                                                     &out_length));
    TEST_ASSERT_EQ_INT(0, out_length);
    return 0;
}

int main(void) {
    TEST_RUN(test_writes_bytes_to_rs485_port);
    TEST_RUN(test_reads_complete_modbus_frame);
    TEST_RUN(test_returns_timeout_when_no_response_arrives);
    return 0;
}
