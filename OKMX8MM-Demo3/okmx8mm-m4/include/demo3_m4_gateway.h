#ifndef DEMO3_M4_GATEWAY_H
#define DEMO3_M4_GATEWAY_H

#include "demo3_m4_modbus.h"

typedef int (*demo3_m4_publish_fn)(void *context, const demo3_sample_t *sample);

typedef struct {
    void *bus_context;
    demo3_m4_modbus_write_fn write;
    demo3_m4_modbus_read_fn read;
    void *publish_context;
    demo3_m4_publish_fn publish;
    uint8_t slave_id;
    uint16_t start_register;
} demo3_m4_gateway_t;

int demo3_m4_gateway_init(demo3_m4_gateway_t *gateway,
                          void *bus_context,
                          demo3_m4_modbus_write_fn write,
                          demo3_m4_modbus_read_fn read,
                          void *publish_context,
                          demo3_m4_publish_fn publish,
                          uint8_t slave_id,
                          uint16_t start_register);
int demo3_m4_gateway_poll(demo3_m4_gateway_t *gateway, demo3_sample_t *sample);

#endif
