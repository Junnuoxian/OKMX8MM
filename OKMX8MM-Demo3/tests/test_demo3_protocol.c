#include <stdint.h>

#include "demo3_protocol.h"

int main(void)
{
    static const uint8_t request[] = {0x01u, 0x03u, 0x00u, 0x00u, 0x00u, 0x0Au};
    demo3_sample_t source = {0};
    demo3_sample_t decoded = {0};
    uint16_t registers[DEMO3_MODBUS_REGISTER_COUNT];

    /* Modbus CRC is transmitted low byte first: C5 CD. */
    if (demo3_crc16_modbus(request, sizeof(request)) != 0xCDC5u) {
        return 1;
    }
    source.sequence = 42u;
    source.analog[0] = -123;
    source.analog[9] = 456;
    source.digital_bits = 3u;
    source.speed_rpm = 900u;
    source.flags = DEMO3_SAMPLE_VALID;
    if (demo3_sample_to_registers(&source, registers,
                                  DEMO3_MODBUS_REGISTER_COUNT) != 0 ||
        demo3_sample_from_registers(registers,
                                    DEMO3_MODBUS_REGISTER_COUNT,
                                    &decoded) != 0) {
        return 2;
    }
    return decoded.sequence == source.sequence &&
                   decoded.analog[0] == source.analog[0] &&
                   decoded.analog[9] == source.analog[9] &&
                   decoded.digital_bits == source.digital_bits &&
                   decoded.speed_rpm == source.speed_rpm &&
                   decoded.flags == source.flags ? 0 : 3;
}
