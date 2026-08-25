#include "bridge/demo3_modbus_adapter.h"

int demo3_modbus_decode_registers(
    const uint16_t *registers,
    size_t register_count,
    demo3_sample_t *sample)
{
    return demo3_sample_from_registers(registers, register_count, sample);
}
