#ifndef DEMO3_MODBUS_CODEC_H
#define DEMO3_MODBUS_CODEC_H

#include <stddef.h>
#include <stdint.h>

#include "../../../common/protocol/demo3_protocol.h"

int demo3_modbus_slave_build_response(const uint8_t *request,
                                      size_t request_length,
                                      const demo3_sample_t *sample,
                                      uint8_t *response,
                                      size_t response_capacity,
                                      size_t *response_length);

#endif
