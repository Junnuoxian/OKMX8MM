#include <stdio.h>
#include <string.h>

#include "runtime/demo3_runtime_status.h"

int main(void)
{
    const char *path = "test_demo3_runtime_status.json";
    demo3_runtime_status_t status;
    FILE *file;
    char text[2048];
    size_t length;

    demo3_runtime_status_init(&status);
    demo3_runtime_status_record_received(&status);
    demo3_runtime_status_record_storage(&status, 0);
    demo3_runtime_status_record_can(&status, 0, 1);
    demo3_runtime_status_record_mqtt(&status, 0, 1);
    demo3_runtime_status_record_ota(&status, 0);
    demo3_runtime_status_error(&status, "test_error");

    if (status.received_frames != 1u || status.stored_samples != 1u ||
        status.can_sent_samples != 1u || status.mqtt_sent_samples != 1u ||
        status.ota_staged != 1u || status.ok != 0 ||
        strcmp(status.last_error, "test_error") != 0) {
        return 1;
    }
    if (demo3_runtime_status_write(path, &status) != 0) {
        return 2;
    }
    file = fopen(path, "rb");
    if (file == 0) {
        return 3;
    }
    length = fread(text, 1u, sizeof(text) - 1u, file);
    fclose(file);
    remove(path);
    text[length] = '\0';
    if (strstr(text, "\"ok\":false") == 0 ||
        strstr(text, "\"received_frames\":1") == 0 ||
        strstr(text, "\"ota_staged\":1") == 0 ||
        strstr(text, "\"last_error\":\"test_error\"") == 0) {
        return 4;
    }
    return 0;
}
