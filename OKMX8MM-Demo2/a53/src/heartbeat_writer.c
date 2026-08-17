#include "a53_demo.h"
#include "file_utils.h"

#include <stdio.h>

int a53_heartbeat_append(const char *path, const a53_m4_batch_t *batch, uint32_t processed_batches)
{
    FILE *file;

    if (path == 0 || batch == 0) {
        return 0;
    }

    file = a53_open_append_text(path);
    if (file == 0) {
        return -1;
    }

    fprintf(file,
        "{\"ok\":true,\"sequence\":%u,\"processed_batches\":%u,"
        "\"sample_rate_hz\":%u,\"digital_bits\":%u}\n",
        batch->sequence,
        processed_batches,
        batch->sample_rate_hz,
        (unsigned)batch->digital_states[batch->sample_count == 0 ? 0 : batch->sample_count - 1]);

    return fclose(file);
}
