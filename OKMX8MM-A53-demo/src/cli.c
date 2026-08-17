#include "a53_demo.h"

#include <stdio.h>
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

static char *trim_text(char *text)
{
    char *end;

    if (text == 0) {
        return 0;
    }
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') {
        text++;
    }
    end = text + strlen(text);
    while (end > text &&
        (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
        *--end = '\0';
    }
    return text;
}

static int copy_option_text(char *target, size_t capacity, const char *value)
{
    size_t length;

    if (target == 0 || capacity == 0 || value == 0) {
        return -1;
    }
    length = strlen(value);
    if (length == 0 || length >= capacity) {
        return -1;
    }
    memcpy(target, value, length + 1);
    return 0;
}

static int apply_config_value(a53_cli_options_t *options, const char *key, const char *value)
{
    if (options == 0 || key == 0 || value == 0) {
        return -1;
    }

    if (strcmp(key, "cycles") == 0) {
        return parse_cycles(value, &options->cycles);
    }
    if (strcmp(key, "source") == 0) {
        if (strcmp(value, "replay") == 0) {
            options->source_kind = A53_SOURCE_REPLAY;
            options->source_path = 0;
            return 0;
        }
        if (strcmp(value, "file") == 0) {
            options->source_kind = A53_SOURCE_FILE;
            return 0;
        }
        return -1;
    }
    if (strcmp(key, "file") == 0 || strcmp(key, "source_path") == 0) {
        if (copy_option_text(options->source_path_value,
                sizeof(options->source_path_value),
                value) != 0) {
            return -1;
        }
        options->source_kind = A53_SOURCE_FILE;
        options->source_path = options->source_path_value;
        return 0;
    }
    if (strcmp(key, "storage") == 0) {
        if (copy_option_text(options->storage_path_value,
                sizeof(options->storage_path_value),
                value) != 0) {
            return -1;
        }
        options->storage_path = options->storage_path_value;
        return 0;
    }
    if (strcmp(key, "mqtt_outbox") == 0) {
        if (copy_option_text(options->mqtt_outbox_path_value,
                sizeof(options->mqtt_outbox_path_value),
                value) != 0) {
            return -1;
        }
        options->mqtt_outbox_path = options->mqtt_outbox_path_value;
        return 0;
    }
    if (strcmp(key, "can_trace") == 0) {
        if (copy_option_text(options->can_trace_path_value,
                sizeof(options->can_trace_path_value),
                value) != 0) {
            return -1;
        }
        options->can_trace_path = options->can_trace_path_value;
        return 0;
    }
    if (strcmp(key, "topic") == 0) {
        if (copy_option_text(options->mqtt_topic_value,
                sizeof(options->mqtt_topic_value),
                value) != 0) {
            return -1;
        }
        options->mqtt_topic = options->mqtt_topic_value;
        return 0;
    }
    if (strcmp(key, "can_id") == 0) {
        return parse_can_id(value, &options->can_id);
    }

    return -1;
}

static int load_config_file(const char *path, a53_cli_options_t *options)
{
    FILE *file;
    char line[512];

    if (path == 0 || options == 0) {
        return -1;
    }

    file = fopen(path, "rb");
    if (file == 0) {
        return -1;
    }

    while (fgets(line, sizeof(line), file) != 0) {
        char *key;
        char *value;
        char *equals;

        key = trim_text(line);
        if (key[0] == '\0' || key[0] == '#') {
            continue;
        }
        equals = strchr(key, '=');
        if (equals == 0) {
            fclose(file);
            return -1;
        }
        *equals = '\0';
        value = trim_text(equals + 1);
        key = trim_text(key);
        if (key[0] == '\0' || value[0] == '\0' ||
            apply_config_value(options, key, value) != 0) {
            fclose(file);
            return -1;
        }
    }

    if (ferror(file) || fclose(file) != 0) {
        return -1;
    }
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
    options->check_storage_path = 0;
    options->recover_storage_path = 0;
    options->cycles = 5;
    options->storage_path = "runtime-data/a53-storage.jsonl";
    options->mqtt_outbox_path = "runtime-data/a53-mqtt-outbox.jsonl";
    options->can_trace_path = "runtime-data/a53-can-trace.log";
    options->mqtt_topic = "mine-truck/demo1";
    options->can_id = 0x321u;
    options->source_path_value[0] = '\0';
    options->storage_path_value[0] = '\0';
    options->mqtt_outbox_path_value[0] = '\0';
    options->can_trace_path_value[0] = '\0';
    options->mqtt_topic_value[0] = '\0';

    for (index = 1; index < argc; index++) {
        if (strcmp(argv[index], "--config") == 0) {
            if (index + 1 >= argc || load_config_file(argv[++index], options) != 0) {
                return -1;
            }
        } else if (strcmp(argv[index], "--file") == 0) {
            if (index + 1 >= argc) {
                return -1;
            }
            options->source_kind = A53_SOURCE_FILE;
            options->source_path = argv[++index];
        } else if (strcmp(argv[index], "--check-storage") == 0) {
            if (index + 1 >= argc) {
                return -1;
            }
            options->check_storage_path = argv[++index];
        } else if (strcmp(argv[index], "--recover-storage") == 0) {
            if (index + 1 >= argc) {
                return -1;
            }
            options->recover_storage_path = argv[++index];
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
