#include "a53_demo.h"
#include "file_utils.h"

#include <stdio.h>

static unsigned int byte_value(unsigned int value)
{
    return value & 0xffu;
}

static unsigned int word_lo(unsigned int value)
{
    return byte_value(value);
}

static unsigned int word_hi(unsigned int value)
{
    return byte_value(value >> 8);
}

int a53_can_trace_append(const char *path, uint32_t can_id, const a53_m4_batch_t *batch)
{
    FILE *file;
    unsigned int sequence;
    unsigned int ai0;
    unsigned int di_bits;
    unsigned int speed_pulse;

    if (path == NULL || batch == NULL) {
        return -1;
    }

    file = a53_open_append_text(path);
    if (file == NULL) {
        return -1;
    }

    sequence = batch->sequence & 0xffffu;
    ai0 = (unsigned int)((uint16_t)batch->analog_samples[0][0]);
    di_bits = batch->digital_states[0];
    speed_pulse = batch->speed_pulse_delta;

    fprintf(file,
        "CAN id=0x%03X seq=%u ai0=%d di=0x%02X speed_pulse=%u speed_period_us=%u "
        "frame=%03X#%02X%02X%02X%02X%02X%02X%02X00\n",
        (unsigned int)can_id,
        batch->sequence,
        batch->analog_samples[0][0],
        batch->digital_states[0],
        batch->speed_pulse_delta,
        batch->speed_period_us,
        (unsigned int)can_id,
        word_lo(sequence),
        word_hi(sequence),
        word_lo(ai0),
        word_hi(ai0),
        byte_value(di_bits),
        word_lo(speed_pulse),
        word_hi(speed_pulse));

    return fclose(file);
}
