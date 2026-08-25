#include <stdio.h>
#include <string.h>

#include "ota/demo3_ota.h"

static int write_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "wb");
    if (file == 0) {
        return -1;
    }
    if (fwrite(text, 1u, strlen(text), file) != strlen(text)) {
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

int main(void)
{
    const char *source = "test_demo3_ota_source.bin";
    const char *staged = "test_demo3_ota_staged.pending";
    const char *marker = "test_demo3_ota_reboot.json";
    FILE *file;
    char text[128] = {0};

    if (write_file(source, "demo3-update-v1") != 0 ||
        demo3_ota_stage_package(source, staged) != 0 ||
        demo3_ota_write_reboot_marker(marker, staged) != 0) {
        remove(source);
        remove(staged);
        remove(marker);
        return 1;
    }
    file = fopen(staged, "rb");
    if (file == 0) {
        return 2;
    }
    (void)fread(text, 1u, sizeof(text) - 1u, file);
    fclose(file);
    if (strcmp(text, "demo3-update-v1") != 0) {
        remove(source);
        remove(staged);
        remove(marker);
        return 3;
    }
    file = fopen(marker, "rb");
    if (file == 0) {
        return 4;
    }
    (void)fread(text, 1u, sizeof(text) - 1u, file);
    fclose(file);
    remove(source);
    remove(staged);
    remove(marker);
    return strstr(text, "reboot-required") == 0 ? 5 : 0;
}
