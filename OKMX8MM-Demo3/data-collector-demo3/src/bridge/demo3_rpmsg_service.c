#define _GNU_SOURCE

#include "demo3_rpmsg_service.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "demo3_rpmsg_linux.h"
#include "demo3_rpmsg_reader.h"
#include "demo3_storage.h"
#include "demo3_can_linux.h"
#include "demo3_mqtt.h"
#include "demo3_mqtt_linux.h"
#include "runtime/demo3_runtime_status.h"
#include "file-storage/file-storage.h"
#include "log/log.h"

#define DEMO3_RPMSG_SERVICE_TEXT_LENGTH 128
#define DEMO3_RPMSG_STORAGE_COUNT 2

typedef struct {
    pthread_t thread;
    char device_path[DEMO3_RPMSG_SERVICE_TEXT_LENGTH];
    char local_storage_path[DEMO3_RPMSG_SERVICE_TEXT_LENGTH];
    char sd_storage_path[DEMO3_RPMSG_SERVICE_TEXT_LENGTH];
    char storage_file_name[DEMO3_RPMSG_SERVICE_TEXT_LENGTH];
    int storage_compress;
    int poll_timeout_ms;
    int can_enabled;
    char can_interface[DEMO3_RPMSG_SERVICE_TEXT_LENGTH];
    uint32_t can_id_base;
    int mqtt_enabled;
    char mqtt_broker[DEMO3_RPMSG_SERVICE_TEXT_LENGTH];
    char mqtt_topic[DEMO3_RPMSG_SERVICE_TEXT_LENGTH];
    char mqtt_client_id[DEMO3_RPMSG_SERVICE_TEXT_LENGTH];
    int mqtt_port;
    int status_enabled;
    char status_path[DEMO3_RPMSG_SERVICE_TEXT_LENGTH];
    int ota_staged;

    demo3_runtime_status_t status;

    demo3_rpmsg_linux_endpoint_t endpoint;
    demo3_can_linux_endpoint_t can_endpoint;
    demo3_mqtt_linux_endpoint_t mqtt_endpoint;
    file_storage_config_t storage_configs[DEMO3_RPMSG_STORAGE_COUNT];
    file_storage_context_t storage_contexts[DEMO3_RPMSG_STORAGE_COUNT];
} demo3_rpmsg_service_context_t;

static int copy_text(char *target, const char *source)
{
    size_t length;

    if (target == 0 || source == 0) {
        return -1;
    }
    length = strlen(source);
    if (length >= DEMO3_RPMSG_SERVICE_TEXT_LENGTH) {
        return -1;
    }
    memcpy(target, source, length + 1u);
    return 0;
}

static int store_sample(void *context, const demo3_sample_t *sample)
{
    demo3_rpmsg_service_context_t *service =
        (demo3_rpmsg_service_context_t *)context;
    int result = 0;
    int storage_result = 0;
    int can_result = 0;
    int mqtt_result = 0;
    int can_was_enabled;
    int mqtt_was_enabled;
    int i;

    demo3_runtime_status_record_received(&service->status);
    for (i = 0; i < DEMO3_RPMSG_STORAGE_COUNT; ++i) {
        if (demo3_store_sample(&service->storage_contexts[i], sample) != 0) {
            storage_result = -1;
        }
    }
    demo3_runtime_status_record_storage(&service->status, storage_result);
    if (storage_result != 0) {
        result = -1;
    }
    can_was_enabled = service->can_enabled;
    if (can_was_enabled &&
        demo3_can_linux_send_sample(&service->can_endpoint, sample) != 0) {
        can_result = -1;
    }
    demo3_runtime_status_record_can(&service->status, can_result,
                                    can_was_enabled);
    if (can_result != 0) {
        result = -1;
    }
    mqtt_was_enabled = service->mqtt_enabled;
    if (service->mqtt_enabled) {
        char payload[2048];
        size_t payload_length = 0u;
        if (demo3_mqtt_build_payload(sample, payload, sizeof(payload),
                                     &payload_length) != 0 ||
            demo3_mqtt_linux_publish(&service->mqtt_endpoint,
                                     service->mqtt_topic,
                                     payload,
                                     payload_length) != 0) {
            m_log(M_LOG_WARN, "MQTT publish failed: %s",
                  service->mqtt_topic);
            demo3_mqtt_linux_close(&service->mqtt_endpoint);
            service->mqtt_enabled = 0;
            mqtt_result = -1;
        }
    }
    demo3_runtime_status_record_mqtt(&service->status, mqtt_result,
                                     mqtt_was_enabled);
    if (mqtt_result != 0) {
        result = -1;
    }
    if (service->status_enabled) {
        (void)demo3_runtime_status_write(service->status_path,
                                         &service->status);
    }
    return result;
}

static int init_storage(demo3_rpmsg_service_context_t *service)
{
    int i;

    service->storage_configs[0].path = service->local_storage_path;
    service->storage_configs[1].path = service->sd_storage_path;
    for (i = 0; i < DEMO3_RPMSG_STORAGE_COUNT; ++i) {
        service->storage_configs[i].file_name = service->storage_file_name;
        service->storage_configs[i].compress =
            i == 0 ? service->storage_compress : 0;
        service->storage_configs[i].rolling_free_space =
            i == 0 ? 500LL * 1024LL * 1024LL : 4LL * 1024LL * 1024LL * 1024LL;
        if (mkdirs(service->storage_configs[i].path) != 0) {
            return -1;
        }
        memset(&service->storage_contexts[i], 0,
               sizeof(service->storage_contexts[i]));
        service->storage_contexts[i].file_storage_config =
            &service->storage_configs[i];
        service->storage_contexts[i].file_fd = -1;
        if (strstr(service->storage_configs[i].path, "/media/sdcard/") != 0 &&
            file_storage_check_sdcard_mount() != 0) {
            service->storage_contexts[i].disabled = 1;
            m_log(M_LOG_WARN, "SD card storage is disabled: %s",
                  service->storage_configs[i].path);
        }
    }
    return 0;
}

static void close_storage(demo3_rpmsg_service_context_t *service)
{
    int i;

    for (i = 0; i < DEMO3_RPMSG_STORAGE_COUNT; ++i) {
        file_storage_close(&service->storage_contexts[i]);
    }
}

static void *run_collector(void *context)
{
    demo3_rpmsg_service_context_t *service =
        (demo3_rpmsg_service_context_t *)context;
    demo3_rpmsg_reader_t reader;
    int result;

    demo3_runtime_status_init(&service->status);
    if (service->ota_staged) {
        demo3_runtime_status_record_ota(&service->status, 0);
    }
    if (service->status_enabled) {
        (void)demo3_runtime_status_write(service->status_path,
                                         &service->status);
    }
    pthread_setname_np(pthread_self(), "DEMO3_RPMSG");
    result = demo3_rpmsg_linux_open(&service->endpoint,
                                    service->device_path,
                                    service->poll_timeout_ms);
    if (result != 0) {
        m_log(M_LOG_ERROR, "Failed to open RPMsg device '%s', ret=%d",
              service->device_path, result);
        demo3_runtime_status_error(&service->status, "rpmsg_open_error");
        if (service->status_enabled) {
            (void)demo3_runtime_status_write(service->status_path,
                                             &service->status);
        }
        free(service);
        return 0;
    }
    if (init_storage(service) != 0) {
        m_log(M_LOG_ERROR, "Failed to initialize RPMsg storage");
        demo3_runtime_status_error(&service->status, "storage_init_error");
        if (service->status_enabled) {
            (void)demo3_runtime_status_write(service->status_path,
                                             &service->status);
        }
        demo3_rpmsg_linux_close(&service->endpoint);
        free(service);
        return 0;
    }
    service->can_endpoint.fd = -1;
    if (service->can_enabled &&
        demo3_can_linux_open(&service->can_endpoint,
                             service->can_interface,
                             service->can_id_base) != 0) {
        m_log(M_LOG_WARN, "CAN output is disabled: %s",
              service->can_interface);
        service->can_enabled = 0;
    }
    service->mqtt_endpoint.fd = -1;
    if (service->mqtt_enabled &&
        demo3_mqtt_linux_open(&service->mqtt_endpoint,
                              service->mqtt_broker,
                              service->mqtt_port,
                              service->mqtt_client_id) != 0) {
        m_log(M_LOG_WARN, "MQTT upload is disabled: %s:%d",
              service->mqtt_broker, service->mqtt_port);
        service->mqtt_enabled = 0;
    }

    reader.context = &service->endpoint;
    reader.read = demo3_rpmsg_linux_read;
    m_log(M_LOG_INFO, "RPMsg collector started on '%s'", service->device_path);
    while (1) {
        result = demo3_rpmsg_reader_step(&reader, service, store_sample);
        if (result < 0) {
            ++service->status.invalid_frames;
            demo3_runtime_status_error(&service->status, "rpmsg_frame_error");
            if (service->status_enabled) {
                (void)demo3_runtime_status_write(service->status_path,
                                                 &service->status);
            }
            m_log(M_LOG_WARN, "RPMsg frame processing failed, ret=%d", result);
            usleep(10000);
        }
    }

    close_storage(service);
    demo3_rpmsg_linux_close(&service->endpoint);
    if (service->can_endpoint.fd >= 0) {
        demo3_can_linux_close(&service->can_endpoint);
    }
    if (service->mqtt_endpoint.fd >= 0) {
        demo3_mqtt_linux_close(&service->mqtt_endpoint);
    }
    free(service);
    return 0;
}

int start_demo3_rpmsg_collector(const demo3_rpmsg_service_config_t *config)
{
    demo3_rpmsg_service_context_t *service;

    if (config == 0 || config->device_path == 0 ||
        config->local_storage_path == 0 || config->sd_storage_path == 0 ||
        config->storage_file_name == 0 || config->can_interface == 0 ||
        config->mqtt_broker == 0 || config->mqtt_topic == 0 ||
        config->mqtt_client_id == 0 || config->status_path == 0 ||
        config->device_path[0] == '\0') {
        return -1;
    }
    service = (demo3_rpmsg_service_context_t *)calloc(1u, sizeof(*service));
    if (service == 0) {
        return -2;
    }
    if (copy_text(service->device_path, config->device_path) != 0 ||
        copy_text(service->local_storage_path, config->local_storage_path) != 0 ||
        copy_text(service->sd_storage_path, config->sd_storage_path) != 0 ||
        copy_text(service->storage_file_name, config->storage_file_name) != 0 ||
        copy_text(service->can_interface, config->can_interface) != 0 ||
        copy_text(service->mqtt_broker, config->mqtt_broker) != 0 ||
        copy_text(service->mqtt_topic, config->mqtt_topic) != 0 ||
        copy_text(service->mqtt_client_id, config->mqtt_client_id) != 0 ||
        copy_text(service->status_path, config->status_path) != 0) {
        free(service);
        return -3;
    }
    service->storage_compress = config->storage_compress;
    service->poll_timeout_ms = config->poll_timeout_ms;
    service->can_enabled = config->can_enabled;
    service->can_id_base = config->can_id_base;
    service->mqtt_enabled = config->mqtt_enabled;
    service->mqtt_port = config->mqtt_port;
    service->status_enabled = config->status_enabled;
    service->ota_staged = config->ota_staged;
    if (pthread_create(&service->thread, 0, run_collector, service) != 0) {
        free(service);
        return -4;
    }
    pthread_detach(service->thread);
    return 0;
}
