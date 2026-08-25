#include "demo3_m4_modbus.h"

#include <string.h>

static int build_read_request(uint8_t slave_id,
                              uint16_t start_register,
                              uint8_t *frame,
                              size_t capacity)
{
    uint16_t crc;

    if (slave_id == 0u || frame == 0 || capacity < 8u) {
        return -1;
    }
    frame[0] = slave_id;
    frame[1] = 0x03u;
    frame[2] = (uint8_t)(start_register >> 8u);
    frame[3] = (uint8_t)(start_register & 0xFFu);
    frame[4] = (uint8_t)(DEMO3_MODBUS_REGISTER_COUNT >> 8u);
    frame[5] = (uint8_t)(DEMO3_MODBUS_REGISTER_COUNT & 0xFFu);
    crc = demo3_crc16_modbus(frame, 6u);
    frame[6] = (uint8_t)(crc & 0xFFu);
    frame[7] = (uint8_t)(crc >> 8u);
    return 8;
}

int demo3_m4_modbus_read_sample(void *context,
                                demo3_m4_modbus_write_fn write,
                                demo3_m4_modbus_read_fn read,
                                uint8_t slave_id,
                                uint16_t start_register,
                                demo3_sample_t *sample)
{
    uint8_t request[8];
    uint8_t response[3u + (DEMO3_MODBUS_REGISTER_COUNT * 2u) + 2u];
    uint16_t registers[DEMO3_MODBUS_REGISTER_COUNT];
    size_t response_length = 0u;
    uint16_t received_crc;
    uint16_t calculated_crc;
    uint8_t byte_count;
    size_t i;

    if (context == 0 || write == 0 || read == 0 || sample == 0) {
        return -1;
    }
    if (build_read_request(slave_id, start_register, request, sizeof(request)) != 8) {
        return -1;
    }
    if (write(context, request, sizeof(request)) != 0) {
        return -2;
    }
    if (read(context, response, sizeof(response), &response_length) != 0) {
        return -3;
    }

    if (response_length != sizeof(response) || response[0] != slave_id || response[1] != 0x03u) {
        return -4;
    }
    byte_count = response[2];
    if (byte_count != DEMO3_MODBUS_REGISTER_COUNT * 2u) {
        return -4;
    }
    received_crc = (uint16_t)response[response_length - 2u] |
                   (uint16_t)((uint16_t)response[response_length - 1u] << 8u);
    calculated_crc = demo3_crc16_modbus(response, (uint32_t)(response_length - 2u));
    if (received_crc != calculated_crc) {
        return -5;
    }
    for (i = 0u; i < DEMO3_MODBUS_REGISTER_COUNT; ++i) {
        size_t offset = 3u + (i * 2u);
        registers[i] = (uint16_t)(((uint16_t)response[offset] << 8u) | response[offset + 1u]);
    }
    return demo3_sample_from_registers(registers, DEMO3_MODBUS_REGISTER_COUNT, sample);
}
