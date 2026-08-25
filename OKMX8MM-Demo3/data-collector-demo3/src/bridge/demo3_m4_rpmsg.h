#ifndef DEMO3_A53_RPMSG_H
#define DEMO3_A53_RPMSG_H

#include <stddef.h>
#include <stdint.h>

#include "demo3_protocol.h"
#include "demo3_rpmsg.h"

int demo3_m4_rpmsg_decode(const uint8_t *frame,
                          size_t frame_length,
                          demo3_sample_t *sample);

#endif
