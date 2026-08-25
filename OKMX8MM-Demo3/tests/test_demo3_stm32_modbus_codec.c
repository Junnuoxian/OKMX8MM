#include <stddef.h>
#include <stdint.h>

#include "demo3_modbus_codec.h"

int main(void)
{
    demo3_sample_t sample = {0};
    uint8_t request[8] = {0x01u, 0x03u, 0x00u, 0x00u, 0x00u, 0x1Au};
    uint8_t response[64];
    size_t response_length = 0u;
    uint16_t crc;
    uint16_t received_crc;

    sample.sequence = 12u;
    sample.analog[0] = -100;
    sample.analog[9] = 900;
    sample.digital_bits = 2u;
    sample.speed_rpm = 1500u;
    sample.flags = DEMO3_SAMPLE_VALID;
    crc = demo3_crc16_modbus(request, 6u);
    request[6] = (uint8_t)(crc & 0xFFu);
    request[7] = (uint8_t)(crc >> 8u);

    if (demo3_modbus_slave_build_response(request, sizeof(request), &sample,
                                          response, sizeof(response),
                                          &response_length) != 0) {
        return 1;
    }
    received_crc = (uint16_t)response[response_length - 2u] |
                   (uint16_t)((uint16_t)response[response_length - 1u] << 8u);
    if (response_length != 57u || response[0] != 0x01u ||
        response[1] != 0x03u || response[2] != 52u ||
        demo3_crc16_modbus(response, (uint32_t)(response_length - 2u)) !=
            received_crc) {
        return 2;
    }
    request[7] ^= 0x01u;
    return demo3_modbus_slave_build_response(request, sizeof(request), &sample,
                                             response, sizeof(response),
                                             &response_length) == -3 ? 0 : 3;
}
