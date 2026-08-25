#ifndef DEMO3_RPMSG_LINUX_H
#define DEMO3_RPMSG_LINUX_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    int fd;
    int poll_timeout_ms;
} demo3_rpmsg_linux_endpoint_t;

int demo3_rpmsg_linux_open(demo3_rpmsg_linux_endpoint_t *endpoint,
                           const char *device_path,
                           int poll_timeout_ms);

int demo3_rpmsg_linux_read(void *context,
                           uint8_t *frame,
                           size_t capacity);

int demo3_rpmsg_linux_close(demo3_rpmsg_linux_endpoint_t *endpoint);

#endif
