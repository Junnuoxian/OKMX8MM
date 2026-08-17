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

static uint32_t checksum_fnv1a(const char *text, size_t length)
{
    uint32_t hash = 2166136261u;
    size_t index;

    for (index = 0; index < length; index++) {
        hash ^= (unsigned char)text[index];
        hash *= 16777619u;
    }

    return hash;
}

static int write_storage_cursor(const char *path,
                                uint32_t sequence,
                                long byte_offset,
                                long line_bytes,
                                uint32_t line_checksum)
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
        "file=%s\nsequence=%u\nbyte_offset=%ld\nline_bytes=%ld\nline_checksum=%08X\n",
        file_name,
        sequence,
        byte_offset,
        line_bytes,
        (unsigned int)line_checksum);

    return fclose(file);
}

int a53_storage_append_batch(const char *path, const a53_m4_batch_t *batch)
{
    FILE *file;
    int channel;
    long start_offset;
    long end_offset;
    char line[1024];
    int used;
    size_t line_length;

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

    used = snprintf(line, sizeof(line),
        "{\"sequence\":%u,\"source\":\"m4-replay\",\"sample_rate_hz\":%u,"
        "\"samples\":%u,\"di_bits\":%u,\"speed_pulse_delta\":%u,"
        "\"speed_period_us\":%u,\"first_sample\":{",
        batch->sequence,
        batch->sample_rate_hz,
        batch->sample_count,
        batch->digital_states[0],
        batch->speed_pulse_delta,
        batch->speed_period_us);
    if (used < 0 || (size_t)used >= sizeof(line)) {
        fclose(file);
        return -1;
    }
    line_length = (size_t)used;

    for (channel = 0; channel < batch->analog_channel_count; channel++) {
        used = snprintf(line + line_length,
            sizeof(line) - line_length,
            "\"ai%d\":%d",
            channel,
            batch->analog_samples[0][channel]);
        if (used < 0 || (size_t)used >= sizeof(line) - line_length) {
            fclose(file);
            return -1;
        }
        line_length += (size_t)used;
        if (channel + 1 < batch->analog_channel_count) {
            if (line_length + 1 >= sizeof(line)) {
                fclose(file);
                return -1;
            }
            line[line_length++] = ',';
            line[line_length] = '\0';
        }
    }

    used = snprintf(line + line_length, sizeof(line) - line_length, "}}\n");
    if (used < 0 || (size_t)used >= sizeof(line) - line_length) {
        fclose(file);
        return -1;
    }
    line_length += (size_t)used;

    if (fwrite(line, 1, line_length, file) != line_length) {
        fclose(file);
        return -1;
    }
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
        end_offset - start_offset,
        checksum_fnv1a(line, line_length));
}
