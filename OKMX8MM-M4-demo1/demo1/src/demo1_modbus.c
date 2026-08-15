#include "demo1_modbus.h"

#include <string.h>

uint16_t demo1_modbus_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFFU;

    if (data == NULL && length > 0U) {
        return 0U;
    }

    for (size_t index = 0U; index < length; ++index) {
        crc ^= (uint16_t)data[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            if ((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc = (uint16_t)(crc >> 1U);
            }
        }
    }

    return crc;
}

int demo1_modbus_build_read_request(uint8_t slave_id,
                                    uint16_t start_register,
                                    uint16_t register_count,
                                    uint8_t *out_frame,
                                    size_t out_capacity) {
    uint16_t crc;

    if (slave_id == 0U || register_count == 0U || register_count > 125U ||
        out_frame == NULL || out_capacity < DEMO1_MODBUS_READ_REQUEST_LENGTH) {
        return -1;
    }

    out_frame[0] = slave_id;
    out_frame[1] = DEMO1_MODBUS_FUNCTION_READ_HOLDING_REGISTERS;
    out_frame[2] = (uint8_t)(start_register >> 8U);
    out_frame[3] = (uint8_t)(start_register & 0xFFU);
    out_frame[4] = (uint8_t)(register_count >> 8U);
    out_frame[5] = (uint8_t)(register_count & 0xFFU);
    crc = demo1_modbus_crc16(out_frame, 6U);
    out_frame[6] = (uint8_t)(crc & 0xFFU);
    out_frame[7] = (uint8_t)(crc >> 8U);

    return DEMO1_MODBUS_READ_REQUEST_LENGTH;
}

int demo1_modbus_parse_read_response(uint8_t slave_id,
                                     const uint8_t *frame,
                                     size_t frame_length,
                                     uint16_t *out_registers,
                                     size_t max_registers,
                                     uint16_t *out_count) {
    uint8_t byte_count;
    uint16_t expected_crc;
    uint16_t received_crc;
    uint16_t register_count;

    if (out_count != NULL) {
        *out_count = 0U;
    }
    if (slave_id == 0U || frame == NULL || out_registers == NULL || out_count == NULL ||
        frame_length < 5U) {
        return -1;
    }

    expected_crc = demo1_modbus_crc16(frame, frame_length - 2U);
    received_crc = (uint16_t)frame[frame_length - 2U] |
                   (uint16_t)((uint16_t)frame[frame_length - 1U] << 8U);
    if (expected_crc != received_crc) {
        return -3;
    }
    if (frame[0] != slave_id ||
        frame[1] != DEMO1_MODBUS_FUNCTION_READ_HOLDING_REGISTERS) {
        return -4;
    }

    byte_count = frame[2];
    if ((byte_count % 2U) != 0U || frame_length != ((size_t)byte_count + 5U)) {
        return -2;
    }

    register_count = (uint16_t)(byte_count / 2U);
    if ((size_t)register_count > max_registers) {
        return -6;
    }

    for (uint16_t index = 0U; index < register_count; ++index) {
        size_t offset = 3U + ((size_t)index * 2U);
        out_registers[index] = (uint16_t)((uint16_t)frame[offset] << 8U) |
                               (uint16_t)frame[offset + 1U];
    }
    *out_count = register_count;
    return 0;
}

int demo1_modbus_registers_to_tick_sample(const uint16_t *registers,
                                          uint16_t register_count,
                                          demo1_tick_sample_t *out_sample) {
    if (registers == NULL || out_sample == NULL ||
        register_count > DEMO1_ANALOG_CHANNEL_COUNT) {
        return -1;
    }

    memset(out_sample, 0, sizeof(*out_sample));
    out_sample->analog_channel_count = (uint8_t)register_count;
    if (register_count == 0U) {
        out_sample->valid_mask = 0U;
        return 0;
    }
    out_sample->valid_mask = (uint16_t)((1U << register_count) - 1U);
    for (uint16_t index = 0U; index < register_count; ++index) {
        out_sample->analog[index] = (int16_t)registers[index];
    }
    return 0;
}
