#include "a53_demo.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    a53_cli_options_t options;
    a53_pipeline_config_t config;

    if (a53_cli_parse(argc, (const char **)argv, &options) != 0) {
        fputs("Usage: okmx8mm-a53-demo [cycles] [--cycles N] [--file input.csv]\n", stderr);
        fputs("       [--check-storage storage.jsonl] [--recover-storage storage.jsonl]\n", stderr);
        fputs("       [--storage file] [--mqtt-outbox file] [--can-trace file]\n", stderr);
        fputs("       [--topic name] [--can-id 0x321]\n", stderr);
        return 2;
    }

    if (options.check_storage_path != 0) {
        if (a53_storage_validate_cursor(options.check_storage_path) != 0) {
            fprintf(stderr, "storage cursor check failed: %s\n", options.check_storage_path);
            return 1;
        }
        printf("storage cursor check passed: %s\n", options.check_storage_path);
        return 0;
    }

    if (options.recover_storage_path != 0) {
        if (a53_storage_recover_tail(options.recover_storage_path) != 0) {
            fprintf(stderr, "storage tail recover failed: %s\n", options.recover_storage_path);
            return 1;
        }
        printf("storage tail recover passed: %s\n", options.recover_storage_path);
        return 0;
    }

    config.source_kind = options.source_kind;
    config.source_path = options.source_path;
    config.storage_path = options.storage_path;
    config.mqtt_outbox_path = options.mqtt_outbox_path;
    config.can_trace_path = options.can_trace_path;
    config.mqtt_topic = options.mqtt_topic;
    config.can_id = options.can_id;
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
