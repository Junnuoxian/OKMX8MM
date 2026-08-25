#ifndef DEMO3_M4_RPMSG_H
#define DEMO3_M4_RPMSG_H

#include <stddef.h>
#include <stdint.h>

#include "demo3_protocol.h"
#include "demo3_rpmsg.h"

#define DEMO3_M4_RPMSG_MAGIC DEMO3_RPMSG_MAGIC
#define DEMO3_M4_RPMSG_SAMPLE_TYPE DEMO3_RPMSG_SAMPLE_TYPE
#define DEMO3_M4_RPMSG_PAYLOAD_LENGTH DEMO3_RPMSG_PAYLOAD_LENGTH
#define DEMO3_M4_RPMSG_FRAME_LENGTH DEMO3_RPMSG_FRAME_LENGTH

typedef int (*demo3_m4_rpmsg_send_fn)(void *context,
                                      const uint8_t *data,
                                      size_t length);

typedef struct {
    void *context;
    demo3_m4_rpmsg_send_fn send;
} demo3_m4_rpmsg_endpoint_t;

int demo3_m4_rpmsg_publish(void *context, const demo3_sample_t *sample);

#endif
