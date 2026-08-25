#ifndef DEMO3_PROTOCOL_H
#define DEMO3_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define DEMO3_ANALOG_CHANNEL_COUNT 10u
#define DEMO3_PROTOCOL_VERSION 1u
#define DEMO3_MODBUS_REGISTER_COUNT 26u

typedef struct {
    uint32_t sequence;
    uint32_t timestamp_ms;
    int32_t analog[DEMO3_ANALOG_CHANNEL_COUNT];
    uint16_t digital_bits;
    uint32_t speed_rpm;
    uint16_t flags;
} demo3_sample_t;

enum {
    DEMO3_SAMPLE_VALID = 1u << 0,
    DEMO3_SAMPLE_ANALOG_ERROR = 1u << 1,
    DEMO3_SAMPLE_DIGITAL_ERROR = 1u << 2,
    DEMO3_SAMPLE_SPEED_ERROR = 1u << 3,
    DEMO3_SAMPLE_CAN_ERROR = 1u << 4,
    DEMO3_SAMPLE_RS485_ERROR = 1u << 5
};

uint16_t demo3_crc16_modbus(const uint8_t *data, uint32_t length);
int demo3_sample_from_registers(const uint16_t *registers,
                                size_t register_count,
                                demo3_sample_t *sample);
int demo3_sample_to_registers(const demo3_sample_t *sample,
                              uint16_t *registers,
                              size_t register_count);

#endif
