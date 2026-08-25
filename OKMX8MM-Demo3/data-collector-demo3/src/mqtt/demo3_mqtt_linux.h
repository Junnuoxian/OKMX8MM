#ifndef DEMO3_MQTT_LINUX_H
#define DEMO3_MQTT_LINUX_H

#include <stddef.h>

typedef struct {
    int fd;
} demo3_mqtt_linux_endpoint_t;

int demo3_mqtt_linux_open(demo3_mqtt_linux_endpoint_t *endpoint,
                          const char *broker_host,
                          int broker_port,
                          const char *client_id);

int demo3_mqtt_linux_publish(demo3_mqtt_linux_endpoint_t *endpoint,
                             const char *topic,
                             const char *payload,
                             size_t payload_length);

int demo3_mqtt_linux_close(demo3_mqtt_linux_endpoint_t *endpoint);

#endif
