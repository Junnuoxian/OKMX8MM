#ifndef DEMO3_RUNTIME_STATUS_H
#define DEMO3_RUNTIME_STATUS_H

#include <stdint.h>

#define DEMO3_RUNTIME_STATUS_ERROR_LENGTH 128

typedef struct {
    int ok;
    uint32_t received_frames;
    uint32_t stored_samples;
    uint32_t can_sent_samples;
    uint32_t mqtt_sent_samples;
    uint32_t ota_staged;
    uint32_t invalid_frames;
    uint32_t storage_errors;
    uint32_t can_errors;
    uint32_t mqtt_errors;
    uint32_t ota_errors;
    char last_error[DEMO3_RUNTIME_STATUS_ERROR_LENGTH];
} demo3_runtime_status_t;

void demo3_runtime_status_init(demo3_runtime_status_t *status);
void demo3_runtime_status_record_received(demo3_runtime_status_t *status);
void demo3_runtime_status_record_storage(demo3_runtime_status_t *status,
                                         int result);
void demo3_runtime_status_record_can(demo3_runtime_status_t *status,
                                     int result,
                                     int enabled);
void demo3_runtime_status_record_mqtt(demo3_runtime_status_t *status,
                                      int result,
                                      int enabled);
void demo3_runtime_status_record_ota(demo3_runtime_status_t *status,
                                     int result);
void demo3_runtime_status_error(demo3_runtime_status_t *status,
                                const char *message);
int demo3_runtime_status_write(const char *path,
                               const demo3_runtime_status_t *status);

#endif
