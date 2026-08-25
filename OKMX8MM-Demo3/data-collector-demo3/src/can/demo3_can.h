#ifndef DEMO3_CAN_H
#define DEMO3_CAN_H

#include <stdint.h>

#include "demo3_protocol.h"

#define DEMO3_CAN_ANALOG_FRAME_COUNT 5u
#define DEMO3_CAN_METADATA_FRAME_INDEX 5u
#define DEMO3_CAN_FRAME_COUNT 6u

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} demo3_can_frame_t;

int demo3_can_encode_sample(const demo3_sample_t *sample,
                            uint32_t base_id,
                            uint8_t frame_index,
                            demo3_can_frame_t *frame);

#endif
