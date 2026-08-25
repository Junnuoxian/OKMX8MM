#ifndef DEMO3_CAN_LINUX_H
#define DEMO3_CAN_LINUX_H

#include <stdint.h>

#include "demo3_protocol.h"

typedef struct {
    int fd;
    uint32_t base_id;
} demo3_can_linux_endpoint_t;

int demo3_can_linux_open(demo3_can_linux_endpoint_t *endpoint,
                         const char *interface_name,
                         uint32_t base_id);

int demo3_can_linux_send_sample(demo3_can_linux_endpoint_t *endpoint,
                                const demo3_sample_t *sample);

int demo3_can_linux_close(demo3_can_linux_endpoint_t *endpoint);

#endif
