#include "demo3_ota.h"

#include <stdio.h>
#include <string.h>

static int make_temporary_path(char *target,
                               size_t capacity,
                               const char *path)
{
    if (target == 0 || path == 0 || path[0] == '\0' ||
        strlen(path) + 5u >= capacity) {
        return -1;
    }
    (void)snprintf(target, capacity, "%s.tmp", path);
    return 0;
}

int demo3_ota_stage_package(const char *package_path,
                            const char *staging_path)
{
    FILE *source;
    FILE *staged;
    char temporary_path[512];
    unsigned char buffer[64u * 1024u];
    size_t count;
    int result = 0;

    if (package_path == 0 || staging_path == 0 ||
        package_path[0] == '\0' || staging_path[0] == '\0' ||
        strcmp(package_path, staging_path) == 0 ||
        make_temporary_path(temporary_path, sizeof(temporary_path),
                             staging_path) != 0) {
        return -1;
    }
    source = fopen(package_path, "rb");
    if (source == 0) {
        return -2;
    }
    staged = fopen(temporary_path, "wb");
    if (staged == 0) {
        fclose(source);
        return -3;
    }
    while ((count = fread(buffer, 1u, sizeof(buffer), source)) > 0u) {
        if (fwrite(buffer, 1u, count, staged) != count) {
            result = -4;
            break;
        }
    }
    if (ferror(source) != 0) {
        result = -5;
    }
    if (fclose(source) != 0 && result == 0) {
        result = -6;
    }
    if (fclose(staged) != 0 && result == 0) {
        result = -7;
    }
    if (result != 0 || rename(temporary_path, staging_path) != 0) {
        remove(temporary_path);
        return result != 0 ? result : -8;
    }
    return 0;
}

int demo3_ota_write_reboot_marker(const char *marker_path,
                                  const char *staging_path)
{
    FILE *file;
    char temporary_path[512];
    int write_result;
    int close_result;

    if (marker_path == 0 || staging_path == 0 || staging_path[0] == '\0' ||
        make_temporary_path(temporary_path, sizeof(temporary_path),
                             marker_path) != 0) {
        return -1;
    }
    file = fopen(temporary_path, "wb");
    if (file == 0) {
        return -2;
    }
    write_result = fprintf(file,
                           "{\"action\":\"reboot-required\","
                           "\"staged_package\":\"%s\"}\n",
                           staging_path);
    close_result = fclose(file);
    if (write_result < 0 || close_result != 0) {
        remove(temporary_path);
        return -3;
    }
    if (rename(temporary_path, marker_path) != 0) {
        remove(temporary_path);
        return -4;
    }
    return 0;
}
