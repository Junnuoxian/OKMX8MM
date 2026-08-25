#include "demo3_protocol.h"

#include <string.h>

uint16_t demo3_crc16_modbus(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFFu;
    uint32_t i;

    if (data == 0) {
        return 0u;
    }

    for (i = 0u; i < length; ++i) {
        uint8_t bit;
        crc ^= data[i];
        for (bit = 0u; bit < 8u; ++bit) {
            if ((crc & 1u) != 0u) {
                crc = (uint16_t)((crc >> 1u) ^ 0xA001u);
            } else {
                crc = (uint16_t)(crc >> 1u);
            }
        }
    }

    return crc;
}

static int32_t join_signed32(uint16_t high, uint16_t low)
{
    uint32_t value = ((uint32_t)high << 16u) | (uint32_t)low;
    return (int32_t)value;
}

static uint32_t join_unsigned32(uint16_t high, uint16_t low)
{
    return ((uint32_t)high << 16u) | (uint32_t)low;
}

static void split_signed32(int32_t value, uint16_t *high, uint16_t *low)
{
    uint32_t raw = (uint32_t)value;
    *high = (uint16_t)(raw >> 16u);
    *low = (uint16_t)(raw & 0xFFFFu);
}

static void split_unsigned32(uint32_t value, uint16_t *high, uint16_t *low)
{
    *high = (uint16_t)(value >> 16u);
    *low = (uint16_t)(value & 0xFFFFu);
}

int demo3_sample_from_registers(const uint16_t *registers,
                                size_t register_count,
                                demo3_sample_t *sample)
{
    size_t channel;

    if (registers == 0 || sample == 0) {
        return -1;
    }
    if (register_count < DEMO3_MODBUS_REGISTER_COUNT) {
        return -2;
    }

    memset(sample, 0, sizeof(*sample));
    for (channel = 0u; channel < DEMO3_ANALOG_CHANNEL_COUNT; ++channel) {
        sample->analog[channel] = join_signed32(
            registers[channel * 2u],
            registers[channel * 2u + 1u]);
    }
    sample->digital_bits = registers[20];
    sample->speed_rpm = join_unsigned32(registers[21], registers[22]);
    sample->flags = registers[23];
    sample->sequence = join_unsigned32(registers[24], registers[25]);
    return 0;
}

int demo3_sample_to_registers(const demo3_sample_t *sample,
                              uint16_t *registers,
                              size_t register_count)
{
    size_t channel;

    if (sample == 0 || registers == 0) {
        return -1;
    }
    if (register_count < DEMO3_MODBUS_REGISTER_COUNT) {
        return -2;
    }
    for (channel = 0u; channel < DEMO3_ANALOG_CHANNEL_COUNT; ++channel) {
        split_signed32(sample->analog[channel],
                       &registers[channel * 2u],
                       &registers[channel * 2u + 1u]);
    }
    registers[20] = sample->digital_bits;
    split_unsigned32(sample->speed_rpm, &registers[21], &registers[22]);
    registers[23] = sample->flags;
    split_unsigned32(sample->sequence, &registers[24], &registers[25]);
    return 0;
}
