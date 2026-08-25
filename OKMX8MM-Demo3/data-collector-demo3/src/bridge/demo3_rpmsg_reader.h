#ifndef DEMO3_RPMSG_READER_H
#define DEMO3_RPMSG_READER_H

#include <stddef.h>
#include <stdint.h>

#include "demo3_rpmsg_bridge.h"

typedef int (*demo3_rpmsg_read_fn)(void *context,
                                   uint8_t *frame,
                                   size_t capacity);

typedef struct {
    void *context;
    demo3_rpmsg_read_fn read;
} demo3_rpmsg_reader_t;

int demo3_rpmsg_reader_step(const demo3_rpmsg_reader_t *reader,
                            void *sink_context,
                            demo3_sample_sink_fn sink);

#endif
