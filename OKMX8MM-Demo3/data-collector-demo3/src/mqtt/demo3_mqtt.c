#include "demo3_mqtt.h"

#include <stdarg.h>
#include <stdio.h>

static int append_text(char *buffer,
                       size_t capacity,
                       size_t *length,
                       const char *format,
                       ...)
{
    va_list arguments;
    int written;

    va_start(arguments, format);
    written = vsnprintf(buffer + *length,
                        capacity > *length ? capacity - *length : 0u,
                        format,
                        arguments);
    va_end(arguments);
    if (written < 0 || *length >= capacity ||
        (size_t)written >= capacity - *length) {
        return -1;
    }
    *length += (size_t)written;
    return 0;
}

int demo3_mqtt_build_payload(const demo3_sample_t *sample,
                             char *buffer,
                             size_t capacity,
                             size_t *length)
{
    size_t channel;

    if (sample == 0 || buffer == 0 || length == 0 || capacity == 0u) {
        return -1;
    }
    *length = 0u;
    if (append_text(buffer, capacity, length,
                    "{\"sequence\":%u,\"timestamp_ms\":%u,\"analog\":[",
                    (unsigned int)sample->sequence,
                    (unsigned int)sample->timestamp_ms) != 0) {
        return -2;
    }
    for (channel = 0u; channel < DEMO3_ANALOG_CHANNEL_COUNT; ++channel) {
        if (append_text(buffer, capacity, length,
                        channel == 0u ? "%d" : ",%d",
                        (int)sample->analog[channel]) != 0) {
            return -2;
        }
    }
    if (append_text(buffer, capacity, length,
                    "],\"digital_bits\":%u,\"speed_rpm\":%u,\"flags\":%u}",
                    (unsigned int)sample->digital_bits,
                    (unsigned int)sample->speed_rpm,
                    (unsigned int)sample->flags) != 0) {
        return -2;
    }
    return 0;
}
