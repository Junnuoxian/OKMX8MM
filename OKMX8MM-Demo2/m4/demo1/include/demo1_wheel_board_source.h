#ifndef DEMO1_WHEEL_BOARD_SOURCE_H
#define DEMO1_WHEEL_BOARD_SOURCE_H

#include <stdint.h>

#include "demo1_modbus_source.h"
#include "demo1_types.h"

enum {
    DEMO1_WHEEL_AI_CHANNEL_COUNT = 8,
    DEMO1_WHEEL_DI_CHANNEL_COUNT = 2,
    DEMO1_WHEEL_SPEED_CHANNEL_COUNT = 1,
    DEMO1_WHEEL_MODBUS_REGISTER_COUNT = 10,
    DEMO1_WHEEL_MODBUS_DI_REGISTER_INDEX = 8,
    DEMO1_WHEEL_MODBUS_SPEED_REGISTER_INDEX = 9
};

typedef struct {
    void *transport_context;
    demo1_modbus_transport_write_fn write;
    demo1_modbus_transport_read_fn read;
    uint8_t slave_id;
    uint16_t start_register;
} demo1_wheel_board_source_t;

int demo1_wheel_board_source_init(demo1_wheel_board_source_t *source,
                                  void *transport_context,
                                  demo1_modbus_transport_write_fn write,
                                  demo1_modbus_transport_read_fn read,
                                  uint8_t slave_id,
                                  uint16_t start_register);
int demo1_wheel_board_source_read_tick(void *context,
                                       uint64_t timestamp_us,
                                       demo1_tick_sample_t *out_sample);

#endif
