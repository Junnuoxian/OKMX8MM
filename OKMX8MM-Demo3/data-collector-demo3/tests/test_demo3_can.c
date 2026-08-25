#include <stdint.h>

#include "demo3_can.h"

int main(void)
{
    demo3_sample_t sample = {0};
    demo3_can_frame_t frame;

    sample.analog[0] = -1;
    sample.analog[1] = (int32_t)0x12345678u;
    sample.digital_bits = 0x1234u;
    sample.speed_rpm = 0x55667788u;
    sample.flags = 0x3344u;

    if (demo3_can_encode_sample(&sample, 0x300u, 0u, &frame) != 0 ||
        frame.id != 0x300u || frame.dlc != 8u ||
        frame.data[0] != 0xFFu || frame.data[1] != 0xFFu ||
        frame.data[2] != 0xFFu || frame.data[3] != 0xFFu ||
        frame.data[4] != 0x78u || frame.data[5] != 0x56u ||
        frame.data[6] != 0x34u || frame.data[7] != 0x12u) {
        return 1;
    }
    if (demo3_can_encode_sample(&sample, 0x300u,
                                DEMO3_CAN_METADATA_FRAME_INDEX,
                                &frame) != 0 ||
        frame.id != 0x305u || frame.dlc != 8u ||
        frame.data[0] != 0x34u || frame.data[1] != 0x12u ||
        frame.data[2] != 0x88u || frame.data[3] != 0x77u ||
        frame.data[4] != 0x66u || frame.data[5] != 0x55u ||
        frame.data[6] != 0x44u || frame.data[7] != 0x33u) {
        return 2;
    }
    if (demo3_can_encode_sample(&sample, 0x300u,
                                DEMO3_CAN_FRAME_COUNT,
                                &frame) != -2) {
        return 3;
    }
    if (demo3_can_encode_sample(&sample, 0x7FFu, 0u, &frame) != -3) {
        return 4;
    }
    return 0;
}
