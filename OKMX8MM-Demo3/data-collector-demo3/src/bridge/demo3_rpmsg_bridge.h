#ifndef DEMO3_RPMSG_BRIDGE_H
#define DEMO3_RPMSG_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#include "demo3_protocol.h"

typedef int (*demo3_sample_sink_fn)(void *context,
                                    const demo3_sample_t *sample);

int demo3_rpmsg_process_frame(const uint8_t *frame,
                              size_t frame_length,
                              void *context,
                              demo3_sample_sink_fn sink);

#endif
