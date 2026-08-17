#include "a53_demo.h"
#include "test_harness.h"

#include <stdio.h>
#include <string.h>

static int count_lines(const char *path)
{
    FILE *file = fopen(path, "rb");
    int lines = 0;
    int ch;
    if (file == NULL) {
        return -1;
    }
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            lines++;
        }
    }
    fclose(file);
    return lines;
}

static int pipeline_processes_requested_m4_batches(void)
{
    const char *storage_path = "pipeline-storage.jsonl";
    const char *mqtt_path = "pipeline-mqtt-outbox.jsonl";
    const char *can_path = "pipeline-can-trace.log";
    a53_pipeline_config_t config;

    remove(storage_path);
    remove(mqtt_path);
    remove(can_path);

    memset(&config, 0, sizeof(config));
    config.storage_path = storage_path;
    config.mqtt_outbox_path = mqtt_path;
    config.can_trace_path = can_path;
    config.mqtt_topic = "mine-truck/demo1";
    config.can_id = 0x321u;
    config.cycles = 3;

    TEST_ASSERT_EQ_INT(0, a53_pipeline_run(&config));
    TEST_ASSERT_EQ_INT(3, count_lines(storage_path));
    TEST_ASSERT_EQ_INT(3, count_lines(mqtt_path));
    TEST_ASSERT_EQ_INT(3, count_lines(can_path));

    remove(storage_path);
    remove(mqtt_path);
    remove(can_path);
    return 0;
}

int main(void)
{
    TEST_RUN(pipeline_processes_requested_m4_batches);
    return 0;
}
