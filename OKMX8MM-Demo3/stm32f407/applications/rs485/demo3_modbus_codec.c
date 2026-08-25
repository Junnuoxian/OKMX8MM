#include "demo3_modbus_codec.h"

static void put_u16_be(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value >> 8u);
    out[1] = (uint8_t)(value & 0xFFu);
}

int demo3_modbus_slave_build_response(const uint8_t *request,
                                      size_t request_length,
                                      const demo3_sample_t *sample,
                                      uint8_t *response,
                                      size_t response_capacity,
                                      size_t *response_length)
{
    uint16_t registers[DEMO3_MODBUS_REGISTER_COUNT];
    uint16_t crc;
    size_t i;
    size_t length = 3u + DEMO3_MODBUS_REGISTER_COUNT * 2u + 2u;

    if (request == 0 || sample == 0 || response == 0 ||
        response_length == 0 || request_length != 8u ||
        response_capacity < length) {
        return -1;
    }
    if (request[1] != 0x03u || request[2] != 0u || request[3] != 0u ||
        request[4] != 0u || request[5] != 26u) {
        return -2;
    }
    crc = demo3_crc16_modbus(request, 6u);
    if (request[6] != (uint8_t)(crc & 0xFFu) ||
        request[7] != (uint8_t)(crc >> 8u)) {
        return -3;
    }
    if (demo3_sample_to_registers(sample, registers,
                                  DEMO3_MODBUS_REGISTER_COUNT) != 0) {
        return -4;
    }
    response[0] = request[0];
    response[1] = 0x03u;
    response[2] = (uint8_t)(DEMO3_MODBUS_REGISTER_COUNT * 2u);
    for (i = 0u; i < DEMO3_MODBUS_REGISTER_COUNT; ++i) {
        put_u16_be(response + 3u + i * 2u, registers[i]);
    }
    crc = demo3_crc16_modbus(response, (uint32_t)(length - 2u));
    response[length - 2u] = (uint8_t)(crc & 0xFFu);
    response[length - 1u] = (uint8_t)(crc >> 8u);
    *response_length = length;
    return 0;
}
