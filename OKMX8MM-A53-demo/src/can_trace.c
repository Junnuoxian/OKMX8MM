#include "a53_demo.h"
#include "file_utils.h"

#include <stdio.h>

int a53_can_trace_append(const char *path, uint32_t can_id, const a53_m4_batch_t *batch)
{
    FILE *file;

    if (path == NULL || batch == NULL) {
        return -1;
    }

    file = a53_open_append_text(path);
    if (file == NULL) {
        return -1;
    }

    fprintf(file,
        "CAN id=0x%03X seq=%u ai0=%d di=0x%02X speed_pulse=%u speed_period_us=%u\n",
        (unsigned int)can_id,
        batch->sequence,
        batch->analog_samples[0][0],
        batch->digital_states[0],
        batch->speed_pulse_delta,
        batch->speed_period_us);

    return fclose(file);
}
