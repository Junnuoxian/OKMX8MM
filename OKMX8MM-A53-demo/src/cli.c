#include "a53_demo.h"

#include <stdlib.h>
#include <string.h>

static int parse_cycles(const char *text, uint32_t *cycles)
{
    unsigned long value;

    if (text == 0 || cycles == 0) {
        return -1;
    }

    value = strtoul(text, 0, 10);
    if (value == 0 || value > 1000000ul) {
        return -1;
    }

    *cycles = (uint32_t)value;
    return 0;
}

static int parse_can_id(const char *text, uint32_t *can_id)
{
    char *end;
    unsigned long value;

    if (text == 0 || can_id == 0) {
        return -1;
    }

    end = 0;
    value = strtoul(text, &end, 0);
    if (end == text || *end != '\0' || value > 0x7fful) {
        return -1;
    }

    *can_id = (uint32_t)value;
    return 0;
}

int a53_cli_parse(int argc, const char **argv, a53_cli_options_t *options)
{
    int index;

    if (options == 0) {
        return -1;
    }

    options->source_kind = A53_SOURCE_REPLAY;
    options->source_path = 0;
    options->cycles = 5;
    options->storage_path = "runtime-data/a53-storage.jsonl";
    options->mqtt_outbox_path = "runtime-data/a53-mqtt-outbox.jsonl";
    options->can_trace_path = "runtime-data/a53-can-trace.log";
    options->mqtt_topic = "mine-truck/demo1";
    options->can_id = 0x321u;

    for (index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--file") == 0) {
            if (index + 1 >= argc) {
                return -1;
            }
            options->source_kind = A53_SOURCE_FILE;
            options->source_path = argv[++index];
        } else if (strcmp(argv[index], "--cycles") == 0) {
            if (index + 1 >= argc || parse_cycles(argv[++index], &options->cycles) != 0) {
                return -1;
            }
        } else if (strcmp(argv[index], "--storage") == 0) {
            if (index + 1 >= argc) {
                return -1;
            }
            options->storage_path = argv[++index];
        } else if (strcmp(argv[index], "--mqtt-outbox") == 0) {
            if (index + 1 >= argc) {
                return -1;
            }
            options->mqtt_outbox_path = argv[++index];
        } else if (strcmp(argv[index], "--can-trace") == 0) {
            if (index + 1 >= argc) {
                return -1;
            }
            options->can_trace_path = argv[++index];
        } else if (strcmp(argv[index], "--topic") == 0) {
            if (index + 1 >= argc) {
                return -1;
            }
            options->mqtt_topic = argv[++index];
        } else if (strcmp(argv[index], "--can-id") == 0) {
            if (index + 1 >= argc || parse_can_id(argv[++index], &options->can_id) != 0) {
                return -1;
            }
        } else if (index == 1 && parse_cycles(argv[index], &options->cycles) == 0) {
            continue;
        } else {
            return -1;
        }
    }

    if (options->source_kind == A53_SOURCE_FILE && options->source_path == 0) {
        return -1;
    }

    return 0;
}
