#ifndef DEMO1_MODBUS_SOURCE_H
#define DEMO1_MODBUS_SOURCE_H

#include <stddef.h>
#include <stdint.h>

#include "demo1_types.h"

typedef int (*demo1_modbus_transport_write_fn)(void *context,
                                               const uint8_t *data,
                                               size_t length);
typedef int (*demo1_modbus_transport_read_fn)(void *context,
                                              uint8_t *data,
                                              size_t capacity,
                                              size_t *out_length);

typedef struct {
    void *transport_context;
    demo1_modbus_transport_write_fn write;
    demo1_modbus_transport_read_fn read;
    uint8_t slave_id;
    uint16_t start_register;
    uint16_t register_count;
} demo1_modbus_source_t;

int demo1_modbus_source_init(demo1_modbus_source_t *source,
                             void *transport_context,
                             demo1_modbus_transport_write_fn write,
                             demo1_modbus_transport_read_fn read,
                             uint8_t slave_id,
                             uint16_t start_register,
                             uint16_t register_count);
int demo1_modbus_source_read_tick(void *context,
                                  uint64_t timestamp_us,
                                  demo1_tick_sample_t *out_sample);

#endif
