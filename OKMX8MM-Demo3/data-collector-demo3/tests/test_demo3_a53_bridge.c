#include <stdint.h>
#include <string.h>

#include "demo3_m4_rpmsg.h"
#include "demo3_protocol.h"
#include "demo3_rpmsg_bridge.h"

typedef struct {
    demo3_sample_t sample;
    int calls;
} sink_context_t;

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

static int accept_sample(void *context, const demo3_sample_t *sample)
{
    sink_context_t *sink = (sink_context_t *)context;
    sink->sample = *sample;
    sink->calls += 1;
    return 0;
}

static size_t make_frame(uint8_t *frame)
{
    size_t offset = 5u;
    size_t channel;

    memset(frame, 0, DEMO3_RPMSG_FRAME_LENGTH);
    frame[0] = DEMO3_RPMSG_MAGIC;
    frame[1] = DEMO3_PROTOCOL_VERSION;
    frame[2] = DEMO3_RPMSG_SAMPLE_TYPE;
    put_u16_le(frame + 3u, DEMO3_RPMSG_PAYLOAD_LENGTH);
    put_u32_le(frame + offset, 7u);
    offset += 4u;
    put_u32_le(frame + offset, 1234u);
    offset += 4u;
    for (channel = 0u; channel < DEMO3_ANALOG_CHANNEL_COUNT; ++channel) {
        put_u32_le(frame + offset, (uint32_t)(-100 - (int32_t)channel));
        offset += 4u;
    }
    put_u16_le(frame + offset, 0x05u);
    offset += 2u;
    put_u32_le(frame + offset, 88u);
    offset += 4u;
    put_u16_le(frame + offset, DEMO3_SAMPLE_VALID);
    offset += 2u;
    put_u16_le(frame + offset, demo3_crc16_modbus(frame, (uint32_t)offset));
    return offset + 2u;
}

int main(void)
{
    uint8_t frame[DEMO3_RPMSG_FRAME_LENGTH];
    demo3_sample_t sample;
    sink_context_t sink = {0};
    size_t frame_length = make_frame(frame);

    if (demo3_m4_rpmsg_decode(frame, frame_length, &sample) != 0) {
        return 1;
    }
    if (sample.sequence != 7u || sample.timestamp_ms != 1234u ||
        sample.analog[0] != -100 || sample.analog[9] != -109 ||
        sample.digital_bits != 0x05u || sample.speed_rpm != 88u ||
        sample.flags != DEMO3_SAMPLE_VALID) {
        return 2;
    }
    frame[DEMO3_RPMSG_FRAME_LENGTH - 1u] ^= 0x01u;
    if (demo3_m4_rpmsg_decode(frame, frame_length, &sample) != -3) {
        return 3;
    }
    frame_length = make_frame(frame);
    if (demo3_rpmsg_process_frame(frame, frame_length, &sink,
                                  accept_sample) != 0 ||
        sink.calls != 1 || sink.sample.sequence != 7u) {
        return 4;
    }
    return 0;
}
