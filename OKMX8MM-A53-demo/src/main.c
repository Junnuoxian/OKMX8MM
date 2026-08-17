#include "a53_demo.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    a53_pipeline_config_t config;
    unsigned long cycles = 5;

    if (argc > 1) {
        cycles = strtoul(argv[1], 0, 10);
    }
    if (cycles == 0) {
        cycles = 1;
    }

    config.storage_path = "runtime-data/a53-storage.jsonl";
    config.mqtt_outbox_path = "runtime-data/a53-mqtt-outbox.jsonl";
    config.can_trace_path = "runtime-data/a53-can-trace.log";
    config.mqtt_topic = "mine-truck/demo1";
    config.can_id = 0x321u;
    config.cycles = (uint32_t)cycles;

    if (a53_pipeline_run(&config) != 0) {
        fputs("A53 demo run failed\n", stderr);
        return 1;
    }

    printf("A53 demo wrote %lu M4 batches\n", cycles);
    printf("storage: %s\n", config.storage_path);
    printf("mqtt outbox: %s\n", config.mqtt_outbox_path);
    printf("can trace: %s\n", config.can_trace_path);
    return 0;
}
