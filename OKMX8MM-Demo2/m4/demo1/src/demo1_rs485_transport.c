#include "demo1_rs485_transport.h"

enum {
    DEMO1_MODBUS_RESPONSE_HEADER_LENGTH = 3,
    DEMO1_MODBUS_RESPONSE_CRC_LENGTH = 2
};

int demo1_rs485_transport_init(demo1_rs485_transport_t *transport,
                               void *port_context,
                               demo1_rs485_write_bytes_fn write_bytes,
                               demo1_rs485_read_byte_fn read_byte,
                               demo1_rs485_now_us_fn now_us,
                               uint32_t response_timeout_us) {
    if (transport == NULL || port_context == NULL || write_bytes == NULL ||
        read_byte == NULL || now_us == NULL || response_timeout_us == 0U) {
        return -1;
    }

    transport->port_context = port_context;
    transport->write_bytes = write_bytes;
    transport->read_byte = read_byte;
    transport->now_us = now_us;
    transport->response_timeout_us = response_timeout_us;
    return 0;
}

int demo1_rs485_transport_write(void *context, const uint8_t *data, size_t length) {
    demo1_rs485_transport_t *transport = (demo1_rs485_transport_t *)context;

    if (transport == NULL || transport->write_bytes == NULL ||
        data == NULL || length == 0U) {
        return -1;
    }
    return transport->write_bytes(transport->port_context, data, length);
}

int demo1_rs485_transport_read(void *context,
                               uint8_t *data,
                               size_t capacity,
                               size_t *out_length) {
    demo1_rs485_transport_t *transport = (demo1_rs485_transport_t *)context;
    uint64_t start_us;
    size_t received = 0U;
    size_t expected_length = 0U;

    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (transport == NULL || data == NULL || out_length == NULL ||
        capacity == 0U || transport->read_byte == NULL || transport->now_us == NULL) {
        return -1;
    }

    start_us = transport->now_us(transport->port_context);
    while ((transport->now_us(transport->port_context) - start_us) <=
           (uint64_t)transport->response_timeout_us) {
        uint8_t byte = 0U;
        int read_result = transport->read_byte(transport->port_context, &byte);

        if (read_result < 0) {
            return -2;
        }
        if (read_result == 0) {
            continue;
        }
        if (received >= capacity) {
            return -4;
        }

        data[received++] = byte;
        if (received == DEMO1_MODBUS_RESPONSE_HEADER_LENGTH) {
            expected_length = (size_t)data[2] +
                              DEMO1_MODBUS_RESPONSE_HEADER_LENGTH +
                              DEMO1_MODBUS_RESPONSE_CRC_LENGTH;
            if (expected_length > capacity) {
                return -4;
            }
        }
        if (expected_length != 0U && received == expected_length) {
            *out_length = received;
            return 0;
        }
    }

    return -3;
}
