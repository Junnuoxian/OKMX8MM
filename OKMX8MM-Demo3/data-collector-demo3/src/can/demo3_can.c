#include "demo3_can.h"

#include <string.h>

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

int demo3_can_encode_sample(const demo3_sample_t *sample,
                            uint32_t base_id,
                            uint8_t frame_index,
                            demo3_can_frame_t *frame)
{
    size_t channel;

    if (sample == 0 || frame == 0) {
        return -1;
    }
    if (frame_index >= DEMO3_CAN_FRAME_COUNT) {
        return -2;
    }
    if (base_id > 0x7FFu - (DEMO3_CAN_FRAME_COUNT - 1u)) {
        return -3;
    }
    memset(frame, 0, sizeof(*frame));
    frame->id = base_id + frame_index;
    frame->dlc = 8u;
    if (frame_index < DEMO3_CAN_ANALOG_FRAME_COUNT) {
        channel = (size_t)frame_index * 2u;
        put_u32_le(frame->data, (uint32_t)sample->analog[channel]);
        put_u32_le(frame->data + 4u,
                   (uint32_t)sample->analog[channel + 1u]);
    } else {
        put_u16_le(frame->data, sample->digital_bits);
        put_u32_le(frame->data + 2u, sample->speed_rpm);
        put_u16_le(frame->data + 6u, sample->flags);
    }
    return 0;
}
