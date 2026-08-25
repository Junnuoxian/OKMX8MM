#ifndef DEMO3_MQTT_H
#define DEMO3_MQTT_H

#include <stddef.h>

#include "demo3_protocol.h"

int demo3_mqtt_build_payload(const demo3_sample_t *sample,
                             char *buffer,
                             size_t capacity,
                             size_t *length);

#endif
