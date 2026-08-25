#include "demo3_m4_rpmsg.h"

#include <string.h>

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)data[0] | (uint16_t)((uint16_t)data[1] << 8u);
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

int demo3_m4_rpmsg_decode(const uint8_t *frame,
                          size_t frame_length,
                          demo3_sample_t *sample)
{
    uint16_t received_crc;
    size_t offset = 5u;
    size_t channel;

    if (frame == 0 || sample == 0) {
        return -1;
    }
    if (frame_length != DEMO3_RPMSG_FRAME_LENGTH ||
        frame[0] != DEMO3_RPMSG_MAGIC ||
        frame[1] != DEMO3_PROTOCOL_VERSION ||
        frame[2] != DEMO3_RPMSG_SAMPLE_TYPE ||
        read_u16_le(frame + 3u) != DEMO3_RPMSG_PAYLOAD_LENGTH) {
        return -2;
    }

    received_crc = read_u16_le(frame + DEMO3_RPMSG_FRAME_LENGTH - 2u);
    if (demo3_crc16_modbus(frame, DEMO3_RPMSG_FRAME_LENGTH - 2u) !=
        received_crc) {
        return -3;
    }

    memset(sample, 0, sizeof(*sample));
    sample->sequence = read_u32_le(frame + offset);
    offset += 4u;
    sample->timestamp_ms = read_u32_le(frame + offset);
    offset += 4u;
    for (channel = 0u; channel < DEMO3_ANALOG_CHANNEL_COUNT; ++channel) {
        sample->analog[channel] = (int32_t)read_u32_le(frame + offset);
        offset += 4u;
    }
    sample->digital_bits = read_u16_le(frame + offset);
    offset += 2u;
    sample->speed_rpm = read_u32_le(frame + offset);
    offset += 4u;
    sample->flags = read_u16_le(frame + offset);
    return 0;
}
