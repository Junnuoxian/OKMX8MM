#include <stdint.h>

#include "bridge/demo3_modbus_adapter.h"

int main(void)
{
    uint16_t registers[DEMO3_MODBUS_REGISTER_COUNT] = {0};
    demo3_sample_t sample;

    registers[0] = 0x0000u;
    registers[1] = 0x03E8u;
    registers[2] = 0xFFFFu;
    registers[3] = 0xFC18u;
    registers[20] = 0x0005u;
    registers[21] = 0x0000u;
    registers[22] = 0x012Cu;
    registers[23] = DEMO3_SAMPLE_VALID;
    registers[24] = 0x0000u;
    registers[25] = 0x002Au;

    if (demo3_modbus_decode_registers(registers, DEMO3_MODBUS_REGISTER_COUNT, &sample) != 0) {
        return 1;
    }
    if (sample.analog[0] != 1000 || sample.analog[1] != -1000 ||
        sample.digital_bits != 0x0005u || sample.speed_rpm != 300u ||
        sample.flags != DEMO3_SAMPLE_VALID || sample.sequence != 42u) {
        return 2;
    }
    if (demo3_modbus_decode_registers(registers, DEMO3_MODBUS_REGISTER_COUNT - 1u, &sample) != -2) {
        return 3;
    }

    return 0;
}
