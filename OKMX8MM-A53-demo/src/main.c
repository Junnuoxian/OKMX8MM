#include "a53_demo.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    a53_cli_options_t options;
    a53_pipeline_config_t config;

    if (a53_cli_parse(argc, (const char **)argv, &options) != 0) {
        fputs("Usage: okmx8mm-a53-demo [cycles] [--cycles N] [--file m4-input.csv]\n", stderr);
        return 2;
    }

    config.source_kind = options.source_kind;
    config.source_path = options.source_path;
    config.storage_path = "runtime-data/a53-storage.jsonl";
    config.mqtt_outbox_path = "runtime-data/a53-mqtt-outbox.jsonl";
    config.can_trace_path = "runtime-data/a53-can-trace.log";
    config.mqtt_topic = "mine-truck/demo1";
    config.can_id = 0x321u;
    config.cycles = options.cycles;

    if (a53_pipeline_run(&config) != 0) {
        fputs("A53 demo run failed\n", stderr);
        return 1;
    }

    printf("A53 demo wrote %u M4 batches\n", options.cycles);
    printf("storage: %s\n", config.storage_path);
    printf("mqtt outbox: %s\n", config.mqtt_outbox_path);
    printf("can trace: %s\n", config.can_trace_path);
    return 0;
}
