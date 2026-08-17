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

static int pipeline_can_use_file_source_without_touching_outputs(void)
{
    const char *input_path = "pipeline-input.csv";
    const char *storage_path = "pipeline-file-storage.jsonl";
    const char *mqtt_path = "pipeline-file-mqtt-outbox.jsonl";
    const char *can_path = "pipeline-file-can-trace.log";
    a53_pipeline_config_t config;

    remove(input_path);
    remove(storage_path);
    remove(mqtt_path);
    remove(can_path);

    {
        FILE *file = fopen(input_path, "wb");
        TEST_ASSERT_TRUE(file != 0);
        fputs("20,301,302,303,304,305,306,307,308,309,310,2,51,62000\n", file);
        fputs("21,401,402,403,404,405,406,407,408,409,410,1,52,63000\n", file);
        fclose(file);
    }

    memset(&config, 0, sizeof(config));
    config.source_kind = A53_SOURCE_FILE;
    config.source_path = input_path;
    config.storage_path = storage_path;
    config.mqtt_outbox_path = mqtt_path;
    config.can_trace_path = can_path;
    config.mqtt_topic = "mine-truck/demo1";
    config.can_id = 0x321u;
    config.cycles = 2;

    TEST_ASSERT_EQ_INT(0, a53_pipeline_run(&config));
    TEST_ASSERT_EQ_INT(2, count_lines(storage_path));
    TEST_ASSERT_EQ_INT(2, count_lines(mqtt_path));
    TEST_ASSERT_EQ_INT(2, count_lines(can_path));

    remove(input_path);
    remove(storage_path);
    remove(mqtt_path);
    remove(can_path);
    return 0;
}

static int pipeline_rejects_non_continuous_file_sequence(void)
{
    const char *input_path = "pipeline-gap-input.csv";
    const char *storage_path = "pipeline-gap-storage.jsonl";
    const char *mqtt_path = "pipeline-gap-mqtt-outbox.jsonl";
    const char *can_path = "pipeline-gap-can-trace.log";
    a53_pipeline_config_t config;

    remove(input_path);
    remove(storage_path);
    remove(mqtt_path);
    remove(can_path);

    {
        FILE *file = fopen(input_path, "wb");
        TEST_ASSERT_TRUE(file != 0);
        fputs("0,301,302,303,304,305,306,307,308,309,310,2,51,62000\n", file);
        fputs("2,401,402,403,404,405,406,407,408,409,410,1,52,63000\n", file);
        fclose(file);
    }

    memset(&config, 0, sizeof(config));
    config.source_kind = A53_SOURCE_FILE;
    config.source_path = input_path;
    config.storage_path = storage_path;
    config.mqtt_outbox_path = mqtt_path;
    config.can_trace_path = can_path;
    config.mqtt_topic = "mine-truck/demo1";
    config.can_id = 0x321u;
    config.cycles = 2;

    TEST_ASSERT_EQ_INT(-1, a53_pipeline_run(&config));
    TEST_ASSERT_EQ_INT(1, count_lines(storage_path));
    TEST_ASSERT_EQ_INT(1, count_lines(mqtt_path));
    TEST_ASSERT_EQ_INT(1, count_lines(can_path));

    remove(input_path);
    remove(storage_path);
    remove(mqtt_path);
    remove(can_path);
    return 0;
}

int main(void)
{
    TEST_RUN(pipeline_processes_requested_m4_batches);
    TEST_RUN(pipeline_can_use_file_source_without_touching_outputs);
    TEST_RUN(pipeline_rejects_non_continuous_file_sequence);
    return 0;
}
