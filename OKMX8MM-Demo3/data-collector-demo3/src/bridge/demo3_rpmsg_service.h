#ifndef DEMO3_RPMSG_SERVICE_H
#define DEMO3_RPMSG_SERVICE_H

typedef struct {
    const char *device_path;
    const char *local_storage_path;
    const char *sd_storage_path;
    const char *storage_file_name;
    int storage_compress;
    int poll_timeout_ms;
} demo3_rpmsg_service_config_t;

int start_demo3_rpmsg_collector(const demo3_rpmsg_service_config_t *config);

#endif
