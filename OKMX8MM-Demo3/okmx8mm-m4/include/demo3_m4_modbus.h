#ifndef DEMO3_M4_MODBUS_H
#define DEMO3_M4_MODBUS_H

#include <stddef.h>
#include <stdint.h>

#include "demo3_protocol.h"

typedef int (*demo3_m4_modbus_write_fn)(void *context,
                                        const uint8_t *data,
                                        size_t length);
typedef int (*demo3_m4_modbus_read_fn)(void *context,
                                       uint8_t *data,
                                       size_t capacity,
                                       size_t *out_length);

int demo3_m4_modbus_read_sample(void *context,
                                demo3_m4_modbus_write_fn write,
                                demo3_m4_modbus_read_fn read,
                                uint8_t slave_id,
                                uint16_t start_register,
                                demo3_sample_t *sample);

#endif
