#include "a53_demo.h"
#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

typedef struct {
    char file_name[256];
    uint32_t sequence;
    long byte_offset;
    long line_bytes;
    uint32_t line_checksum;
} storage_cursor_info_t;

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

static const char *base_name(const char *path)
{
    const char *file_name;

    file_name = strrchr(path, '/');
    if (file_name == NULL) {
        file_name = strrchr(path, '\\');
    }

    return file_name == NULL ? path : file_name + 1;
}

static int read_storage_cursor(const char *path, storage_cursor_info_t *cursor)
{
    char cursor_path[512];
    char cursor_text[512];
    FILE *cursor_file;
    size_t cursor_size;
    unsigned int sequence;
    long byte_offset;
    long line_bytes;
    unsigned int line_checksum;

    if (path == NULL || cursor == NULL || build_cursor_path(path, cursor_path, sizeof(cursor_path)) != 0) {
        return -1;
    }

    cursor_file = fopen(cursor_path, "rb");
    if (cursor_file == NULL) {
        return -1;
    }
    cursor_size = fread(cursor_text, 1, sizeof(cursor_text) - 1, cursor_file);
    if (ferror(cursor_file) || fclose(cursor_file) != 0 || cursor_size == 0) {
        return -1;
    }
    cursor_text[cursor_size] = '\0';

    if (sscanf(cursor_text,
            "file=%255[^\n]\nsequence=%u\nbyte_offset=%ld\nline_bytes=%ld\nline_checksum=%8X\n",
            cursor->file_name,
            &sequence,
            &byte_offset,
            &line_bytes,
            &line_checksum) != 5) {
        return -1;
    }
    if (strcmp(cursor->file_name, base_name(path)) != 0 ||
        byte_offset <= 0 ||
        line_bytes <= 0 ||
        line_bytes >= 1024 ||
        byte_offset < line_bytes) {
        return -1;
    }
    cursor->sequence = (uint32_t)sequence;
    cursor->byte_offset = byte_offset;
    cursor->line_bytes = line_bytes;
    cursor->line_checksum = (uint32_t)line_checksum;

    return 0;
}

static int file_size(FILE *file, long *size)
{
    if (file == NULL || size == NULL || fseek(file, 0, SEEK_END) != 0) {
        return -1;
    }
    *size = ftell(file);
    return *size < 0 ? -1 : 0;
}

static int truncate_open_file(FILE *file, long offset)
{
    if (file == NULL || offset < 0 || fflush(file) != 0) {
        return -1;
    }

#if defined(_WIN32)
    {
        HANDLE handle;
        LARGE_INTEGER position;

        handle = (HANDLE)_get_osfhandle(_fileno(file));
        if (handle == INVALID_HANDLE_VALUE) {
            return -1;
        }
        position.QuadPart = offset;
        if (SetFilePointerEx(handle, position, NULL, FILE_BEGIN) == 0 ||
            SetEndOfFile(handle) == 0) {
            return -1;
        }
    }
#else
    if (ftruncate(fileno(file), (off_t)offset) != 0) {
        return -1;
    }
#endif
    clearerr(file);
    return fseek(file, offset, SEEK_SET);
}

static int verify_cursor_line(const char *path, const storage_cursor_info_t *cursor)
{
    char expected_sequence[64];
    char line[1024];
    FILE *storage_file;
    size_t read_size;

    if (path == NULL || cursor == NULL) {
        return -1;
    }
    storage_file = fopen(path, "rb");
    if (storage_file == NULL) {
        return -1;
    }
    if (fseek(storage_file, cursor->byte_offset - cursor->line_bytes, SEEK_SET) != 0) {
        fclose(storage_file);
        return -1;
    }
    read_size = fread(line, 1, (size_t)cursor->line_bytes, storage_file);
    if (ferror(storage_file) || fclose(storage_file) != 0 || read_size != (size_t)cursor->line_bytes) {
        return -1;
    }
    line[read_size] = '\0';

    if (checksum_fnv1a(line, read_size) != cursor->line_checksum) {
        return -1;
    }

    if (snprintf(expected_sequence, sizeof(expected_sequence), "\"sequence\":%u", cursor->sequence) < 0) {
        return -1;
    }
    if (strstr(line, expected_sequence) == NULL) {
        return -1;
    }

    return 0;
}

int a53_storage_validate_cursor(const char *path)
{
    storage_cursor_info_t cursor;
    FILE *storage_file;
    long storage_size;

    if (read_storage_cursor(path, &cursor) != 0) {
        return -1;
    }

    storage_file = fopen(path, "rb");
    if (storage_file == NULL) {
        return -1;
    }
    if (file_size(storage_file, &storage_size) != 0 || fclose(storage_file) != 0) {
        return -1;
    }
    if (storage_size != cursor.byte_offset) {
        return -1;
    }

    return verify_cursor_line(path, &cursor);
}

int a53_storage_recover_tail(const char *path)
{
    storage_cursor_info_t cursor;
    FILE *storage_file;
    long storage_size;
    int result;

    if (read_storage_cursor(path, &cursor) != 0 || verify_cursor_line(path, &cursor) != 0) {
        return -1;
    }

    storage_file = fopen(path, "r+b");
    if (storage_file == NULL) {
        return -1;
    }
    if (file_size(storage_file, &storage_size) != 0 || storage_size < cursor.byte_offset) {
        fclose(storage_file);
        return -1;
    }
    result = truncate_open_file(storage_file, cursor.byte_offset);
    if (fclose(storage_file) != 0) {
        return -1;
    }

    return result;
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
