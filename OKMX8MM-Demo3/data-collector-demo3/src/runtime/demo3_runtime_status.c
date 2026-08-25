#include "demo3_runtime_status.h"

#include <stdio.h>
#include <string.h>

static void set_error(demo3_runtime_status_t *status, const char *message)
{
    if (status == 0) {
        return;
    }
    status->ok = 0;
    if (message == 0) {
        status->last_error[0] = '\0';
        return;
    }
    (void)snprintf(status->last_error,
                   sizeof(status->last_error),
                   "%s",
                   message);
}

void demo3_runtime_status_init(demo3_runtime_status_t *status)
{
    if (status == 0) {
        return;
    }
    memset(status, 0, sizeof(*status));
    status->ok = 1;
}

void demo3_runtime_status_record_received(demo3_runtime_status_t *status)
{
    if (status != 0) {
        ++status->received_frames;
    }
}

void demo3_runtime_status_record_storage(demo3_runtime_status_t *status,
                                         int result)
{
    if (status == 0) {
        return;
    }
    if (result == 0) {
        ++status->stored_samples;
    } else {
        ++status->storage_errors;
        set_error(status, "storage_error");
    }
}

void demo3_runtime_status_record_can(demo3_runtime_status_t *status,
                                     int result,
                                     int enabled)
{
    if (status == 0 || !enabled) {
        return;
    }
    if (result == 0) {
        ++status->can_sent_samples;
    } else {
        ++status->can_errors;
        set_error(status, "can_error");
    }
}

void demo3_runtime_status_record_mqtt(demo3_runtime_status_t *status,
                                      int result,
                                      int enabled)
{
    if (status == 0 || !enabled) {
        return;
    }
    if (result == 0) {
        ++status->mqtt_sent_samples;
    } else {
        ++status->mqtt_errors;
        set_error(status, "mqtt_error");
    }
}

void demo3_runtime_status_record_ota(demo3_runtime_status_t *status,
                                     int result)
{
    if (status == 0) {
        return;
    }
    if (result == 0) {
        ++status->ota_staged;
    } else {
        ++status->ota_errors;
        set_error(status, "ota_error");
    }
}

void demo3_runtime_status_error(demo3_runtime_status_t *status,
                                const char *message)
{
    set_error(status, message);
}

static void write_json_string(FILE *file, const char *text)
{
    const unsigned char *cursor = (const unsigned char *)text;

    fputc('"', file);
    if (cursor != 0) {
        while (*cursor != '\0') {
            if (*cursor == '\\' || *cursor == '"') {
                fputc('\\', file);
                fputc(*cursor, file);
            } else if (*cursor == '\n') {
                fputs("\\n", file);
            } else if (*cursor == '\r') {
                fputs("\\r", file);
            } else if (*cursor == '\t') {
                fputs("\\t", file);
            } else if (*cursor < 0x20u) {
                fputc('?', file);
            } else {
                fputc(*cursor, file);
            }
            ++cursor;
        }
    }
    fputc('"', file);
}

int demo3_runtime_status_write(const char *path,
                               const demo3_runtime_status_t *status)
{
    char temporary_path[512];
    FILE *file;

    if (path == 0 || status == 0 || path[0] == '\0' ||
        strlen(path) + 5u >= sizeof(temporary_path)) {
        return -1;
    }
    (void)snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", path);
    file = fopen(temporary_path, "wb");
    if (file == 0) {
        return -2;
    }
    fprintf(file,
            "{\"ok\":%s,\"received_frames\":%u,"
            "\"stored_samples\":%u,\"can_sent_samples\":%u,"
            "\"mqtt_sent_samples\":%u,\"ota_staged\":%u,"
            "\"invalid_frames\":%u,\"storage_errors\":%u,"
            "\"can_errors\":%u,\"mqtt_errors\":%u,"
            "\"ota_errors\":%u,\"last_error\":",
            status->ok ? "true" : "false",
            status->received_frames,
            status->stored_samples,
            status->can_sent_samples,
            status->mqtt_sent_samples,
            status->ota_staged,
            status->invalid_frames,
            status->storage_errors,
            status->can_errors,
            status->mqtt_errors,
            status->ota_errors);
    write_json_string(file, status->last_error);
    fputs("}\n", file);
    if (fclose(file) != 0) {
        remove(temporary_path);
        return -3;
    }
    if (rename(temporary_path, path) != 0) {
        remove(temporary_path);
        return -4;
    }
    return 0;
}
