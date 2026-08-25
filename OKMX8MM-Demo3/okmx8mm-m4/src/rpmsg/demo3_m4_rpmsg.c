#include "demo3_m4_rpmsg.h"

static void put_u16_le(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)(value >> 8u);
}

static void put_u32_le(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8u) & 0xFFu);
    out[2] = (uint8_t)((value >> 16u) & 0xFFu);
    out[3] = (uint8_t)(value >> 24u);
}

int demo3_m4_rpmsg_publish(void *context, const demo3_sample_t *sample)
{
    demo3_m4_rpmsg_endpoint_t *endpoint = (demo3_m4_rpmsg_endpoint_t *)context;
    uint8_t frame[DEMO3_M4_RPMSG_FRAME_LENGTH];
    uint16_t crc;
    size_t offset = 5u;
    size_t channel;

    if (endpoint == 0 || endpoint->send == 0 || sample == 0) {
        return -1;
    }

    frame[0] = DEMO3_M4_RPMSG_MAGIC;
    frame[1] = DEMO3_PROTOCOL_VERSION;
    frame[2] = DEMO3_M4_RPMSG_SAMPLE_TYPE;
    put_u16_le(frame + 3u, DEMO3_M4_RPMSG_PAYLOAD_LENGTH);
    put_u32_le(frame + offset, sample->sequence);
    offset += 4u;
    put_u32_le(frame + offset, sample->timestamp_ms);
    offset += 4u;
    for (channel = 0u; channel < DEMO3_ANALOG_CHANNEL_COUNT; ++channel) {
        put_u32_le(frame + offset, (uint32_t)sample->analog[channel]);
        offset += 4u;
    }
    put_u16_le(frame + offset, sample->digital_bits);
    offset += 2u;
    put_u32_le(frame + offset, sample->speed_rpm);
    offset += 4u;
    put_u16_le(frame + offset, sample->flags);
    offset += 2u;

    crc = demo3_crc16_modbus(frame, (uint32_t)offset);
    put_u16_le(frame + offset, crc);
    return endpoint->send(endpoint->context, frame, sizeof(frame));
}
