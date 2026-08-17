#include "a53_demo.h"
#include "file_utils.h"

#include <stdio.h>
#include <string.h>

static int build_cursor_path(const char *path, char *cursor_path, size_t capacity)
{
    int result;

    if (path == NULL || cursor_path == NULL || capacity == 0) {
        return -1;
    }

    result = snprintf(cursor_path, capacity, "%s.cursor", path);
    if (result < 0 || (size_t)result >= capacity) {
        return -1;
    }

    return 0;
}

static int write_storage_cursor(const char *path,
                                uint32_t sequence,
                                long byte_offset,
                                long line_bytes)
{
    char cursor_path[512];
    FILE *file;
    const char *file_name;

    if (build_cursor_path(path, cursor_path, sizeof(cursor_path)) != 0) {
        return -1;
    }

    file_name = strrchr(path, '/');
    if (file_name == NULL) {
        file_name = strrchr(path, '\\');
    }
    file_name = file_name == NULL ? path : file_name + 1;

    file = a53_open_write_text(cursor_path);
    if (file == NULL) {
        return -1;
    }

    fprintf(file,
        "file=%s\nsequence=%u\nbyte_offset=%ld\nline_bytes=%ld\n",
        file_name,
        sequence,
        byte_offset,
        line_bytes);

    return fclose(file);
}

int a53_storage_append_batch(const char *path, const a53_m4_batch_t *batch)
{
    FILE *file;
    int channel;
    long start_offset;
    long end_offset;

    if (path == NULL || batch == NULL) {
        return -1;
    }

    file = a53_open_append_text(path);
    if (file == NULL) {
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }
    start_offset = ftell(file);
    if (start_offset < 0) {
        fclose(file);
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
    end_offset = ftell(file);
    if (end_offset < 0) {
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        return -1;
    }

    return write_storage_cursor(path,
        batch->sequence,
        end_offset,
        end_offset - start_offset);
}
