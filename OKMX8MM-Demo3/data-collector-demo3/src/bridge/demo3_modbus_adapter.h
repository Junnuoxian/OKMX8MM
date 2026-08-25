#ifndef DEMO3_MODBUS_ADAPTER_H
#define DEMO3_MODBUS_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#include "demo3_protocol.h"

int demo3_modbus_decode_registers(
    const uint16_t *registers,
    size_t register_count,
    demo3_sample_t *sample);

#endif
