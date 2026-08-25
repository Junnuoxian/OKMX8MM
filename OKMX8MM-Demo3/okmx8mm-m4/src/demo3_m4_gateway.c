#include "demo3_m4_gateway.h"

#include <stddef.h>

int demo3_m4_gateway_init(demo3_m4_gateway_t *gateway,
                          void *bus_context,
                          demo3_m4_modbus_write_fn write,
                          demo3_m4_modbus_read_fn read,
                          void *publish_context,
                          demo3_m4_publish_fn publish,
                          uint8_t slave_id,
                          uint16_t start_register)
{
    if (gateway == NULL || bus_context == NULL || write == NULL || read == NULL || slave_id == 0u) {
        return -1;
    }
    gateway->bus_context = bus_context;
    gateway->write = write;
    gateway->read = read;
    gateway->publish_context = publish_context;
    gateway->publish = publish;
    gateway->slave_id = slave_id;
    gateway->start_register = start_register;
    return 0;
}

int demo3_m4_gateway_poll(demo3_m4_gateway_t *gateway, demo3_sample_t *sample)
{
    int read_ret;

    if (gateway == NULL || sample == NULL) {
        return -1;
    }
    read_ret = demo3_m4_modbus_read_sample(
        gateway->bus_context,
        gateway->write,
        gateway->read,
        gateway->slave_id,
        gateway->start_register,
        sample);
    if (read_ret != 0) {
        return read_ret;
    }
    if (gateway->publish != NULL &&
        gateway->publish(gateway->publish_context, sample) != 0) {
        return -6;
    }
    return 0;
}
