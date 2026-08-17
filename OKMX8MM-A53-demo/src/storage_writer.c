#include "a53_demo.h"
#include "file_utils.h"

#include <stdio.h>

int a53_storage_append_batch(const char *path, const a53_m4_batch_t *batch)
{
    FILE *file;
    int channel;

    if (path == NULL || batch == NULL) {
        return -1;
    }

    file = a53_open_append_text(path);
    if (file == NULL) {
        return -1;
    }

    fprintf(file,
        "{\"sequence\":%u,\"source\":\"m4-replay\",\"sample_rate_hz\":%u,"
        "\"samples\":%u,\"di_bits\":%u,\"speed_pulse_delta\":%u,"
        "\"speed_period_us\":%u,\"first_sample\":{",
        batch->sequence,
        batch->sample_rate_hz,
        batch->sample_count,
        batch->digital_states[0],
        batch->speed_pulse_delta,
        batch->speed_period_us);

    for (channel = 0; channel < batch->analog_channel_count; channel++) {
        fprintf(file, "\"ai%d\":%d", channel, batch->analog_samples[0][channel]);
        if (channel + 1 < batch->analog_channel_count) {
            fputc(',', file);
        }
    }

    fputs("}}\n", file);
    return fclose(file);
}
