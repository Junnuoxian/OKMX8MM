#include <stdint.h>
#include <string.h>

#include "demo3_protocol.h"
#include "demo3_rpmsg_reader.h"
#include "demo3_rpmsg.h"

typedef struct {
    uint8_t frame[DEMO3_RPMSG_FRAME_LENGTH];
    int calls;
} mock_channel_t;

typedef struct {
    demo3_sample_t sample;
    int calls;
} mock_sink_t;

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
    out[3] = (uint8_t)((value >> 24u) & 0xFFu);
}

static int mock_read(void *context, uint8_t *frame, size_t capacity)
{
    mock_channel_t *channel = (mock_channel_t *)context;
    if (capacity < sizeof(channel->frame)) {
        return -2;
    }
    memcpy(frame, channel->frame, sizeof(channel->frame));
    channel->calls += 1;
    return (int)sizeof(channel->frame);
}

static int mock_sink(void *context, const demo3_sample_t *sample)
{
    mock_sink_t *sink = (mock_sink_t *)context;
    sink->sample = *sample;
    sink->calls += 1;
    return 0;
}

static void make_frame(uint8_t *frame)
{
    size_t offset = 5u;

    memset(frame, 0, DEMO3_RPMSG_FRAME_LENGTH);
    frame[0] = DEMO3_RPMSG_MAGIC;
    frame[1] = DEMO3_PROTOCOL_VERSION;
    frame[2] = DEMO3_RPMSG_SAMPLE_TYPE;
    put_u16_le(frame + 3u, DEMO3_RPMSG_PAYLOAD_LENGTH);
    put_u32_le(frame + offset, 33u);
    offset += 4u;
    put_u32_le(frame + offset, 44u);
    offset += 4u;
    put_u16_le(frame + offset + 40u, 0x02u);
    put_u32_le(frame + offset + 42u, 66u);
    put_u16_le(frame + offset + 46u, DEMO3_SAMPLE_VALID);
    put_u16_le(frame + offset + 48u,
               demo3_crc16_modbus(frame, (uint32_t)(offset + 48u)));
}

int main(void)
{
    mock_channel_t channel = {0};
    mock_sink_t sink = {0};
    demo3_rpmsg_reader_t reader;

    make_frame(channel.frame);
    reader.context = &channel;
    reader.read = mock_read;
    if (demo3_rpmsg_reader_step(&reader, &sink, mock_sink) != 0) {
        return 1;
    }
    if (channel.calls != 1 || sink.calls != 1 ||
        sink.sample.sequence != 33u || sink.sample.timestamp_ms != 44u ||
        sink.sample.digital_bits != 0x02u || sink.sample.speed_rpm != 66u) {
        return 2;
    }
    return 0;
}
