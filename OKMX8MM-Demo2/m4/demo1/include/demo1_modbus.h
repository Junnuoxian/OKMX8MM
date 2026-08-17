#ifndef DEMO1_MODBUS_H
#define DEMO1_MODBUS_H

#include <stddef.h>
#include <stdint.h>

#include "demo1_types.h"

enum {
    DEMO1_MODBUS_FUNCTION_READ_HOLDING_REGISTERS = 0x03,
    DEMO1_MODBUS_DEFAULT_SLAVE_ID = 1,
    DEMO1_MODBUS_DEFAULT_START_REGISTER = 0,
    DEMO1_MODBUS_DEFAULT_REGISTER_COUNT = 10,
    DEMO1_MODBUS_READ_REQUEST_LENGTH = 8
};

uint16_t demo1_modbus_crc16(const uint8_t *data, size_t length);
int demo1_modbus_build_read_request(uint8_t slave_id,
                                    uint16_t start_register,
                                    uint16_t register_count,
                                    uint8_t *out_frame,
                                    size_t out_capacity);
int demo1_modbus_parse_read_response(uint8_t slave_id,
                                     const uint8_t *frame,
                                     size_t frame_length,
                                     uint16_t *out_registers,
                                     size_t max_registers,
                                     uint16_t *out_count);
int demo1_modbus_registers_to_tick_sample(const uint16_t *registers,
                                          uint16_t register_count,
                                          demo1_tick_sample_t *out_sample);

#endif
