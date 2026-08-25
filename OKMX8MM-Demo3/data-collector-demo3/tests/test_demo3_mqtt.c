#include <stddef.h>
#include <string.h>

#include "demo3_mqtt.h"

int main(void)
{
    demo3_sample_t sample = {0};
    char payload[1024];
    size_t length = 0u;

    sample.sequence = 8u;
    sample.timestamp_ms = 100u;
    sample.analog[0] = -12;
    sample.analog[9] = 90;
    sample.digital_bits = 3u;
    sample.speed_rpm = 1200u;
    sample.flags = DEMO3_SAMPLE_VALID;

    if (demo3_mqtt_build_payload(&sample, payload, sizeof(payload), &length) != 0) {
        return 1;
    }
    if (length == 0u || length >= sizeof(payload) ||
        strstr(payload, "\"sequence\":8") == 0 ||
        strstr(payload, "\"digital_bits\":3") == 0 ||
        strstr(payload, "\"speed_rpm\":1200") == 0 ||
        strstr(payload, "\"analog\":[") == 0) {
        return 2;
    }
    if (demo3_mqtt_build_payload(&sample, payload, 8u, &length) != -2) {
        return 3;
    }
    return 0;
}
