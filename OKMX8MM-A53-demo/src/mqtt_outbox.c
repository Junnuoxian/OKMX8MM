#include "a53_demo.h"
#include "file_utils.h"

#include <stdio.h>

int a53_mqtt_outbox_append(const char *path, const char *topic, const a53_m4_batch_t *batch)
{
    FILE *file;

    if (path == NULL || topic == NULL || batch == NULL) {
        return -1;
    }

    file = a53_open_append_text(path);
    if (file == NULL) {
        return -1;
    }

    fprintf(file,
        "{\"sequence\":%u,\"qos\":%d,\"topic\":\"%s\",\"payload\":{\"ai0\":%d,"
        "\"di_bits\":%u,\"speed_pulse_delta\":%u}}\n",
        batch->sequence,
        A53_MQTT_DEFAULT_QOS,
        topic,
        batch->analog_samples[0][0],
        batch->digital_states[0],
        batch->speed_pulse_delta);

    return fclose(file);
}
