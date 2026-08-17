#include "a53_demo.h"
#include "test_harness.h"

#include <stdio.h>

static int write_text(const char *path, const char *text)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return -1;
    }
    fputs(text, file);
    return fclose(file);
}

static int cli_defaults_to_replay_source(void)
{
    const char *argv[] = {"okmx8mm-a53-demo"};
    a53_cli_options_t options;

    TEST_ASSERT_EQ_INT(0, a53_cli_parse(1, argv, &options));
    TEST_ASSERT_EQ_INT(A53_SOURCE_REPLAY, options.source_kind);
    TEST_ASSERT_EQ_INT(5, options.cycles);
    TEST_ASSERT_TRUE(options.source_path == 0);
    TEST_ASSERT_TRUE(options.check_storage_path == 0);
    TEST_ASSERT_TRUE(options.recover_storage_path == 0);
    TEST_ASSERT_EQ_STR("runtime-data/a53-storage.jsonl", options.storage_path);
    TEST_ASSERT_EQ_STR("runtime-data/a53-mqtt-outbox.jsonl", options.mqtt_outbox_path);
    TEST_ASSERT_EQ_STR("runtime-data/a53-can-trace.log", options.can_trace_path);
    TEST_ASSERT_EQ_STR("runtime-data/a53-status.json", options.status_path);
    TEST_ASSERT_EQ_STR("mine-truck/demo1", options.mqtt_topic);
    TEST_ASSERT_EQ_INT(0x321, options.can_id);
    return 0;
}

static int cli_accepts_storage_cursor_check_mode(void)
{
    const char *argv[] = {
        "okmx8mm-a53-demo",
        "--check-storage",
        "runtime-data/a53-storage.jsonl"
    };
    a53_cli_options_t options;

    TEST_ASSERT_EQ_INT(0, a53_cli_parse(3, argv, &options));
    TEST_ASSERT_EQ_STR("runtime-data/a53-storage.jsonl", options.check_storage_path);
    return 0;
}

static int cli_accepts_storage_tail_recover_mode(void)
{
    const char *argv[] = {
        "okmx8mm-a53-demo",
        "--recover-storage",
        "runtime-data/a53-storage.jsonl"
    };
    a53_cli_options_t options;

    TEST_ASSERT_EQ_INT(0, a53_cli_parse(3, argv, &options));
    TEST_ASSERT_EQ_STR("runtime-data/a53-storage.jsonl", options.recover_storage_path);
    return 0;
}

static int cli_accepts_file_source_and_cycle_count(void)
{
    const char *argv[] = {
        "okmx8mm-a53-demo",
        "--file",
        "m4-input.csv",
        "--cycles",
        "2"
    };
    a53_cli_options_t options;

    TEST_ASSERT_EQ_INT(0, a53_cli_parse(5, argv, &options));
    TEST_ASSERT_EQ_INT(A53_SOURCE_FILE, options.source_kind);
    TEST_ASSERT_EQ_INT(2, options.cycles);
    TEST_ASSERT_EQ_STR("m4-input.csv", options.source_path);
    return 0;
}

static int cli_accepts_output_settings(void)
{
    const char *argv[] = {
        "okmx8mm-a53-demo",
        "--storage",
        "/mnt/sdcard/samples.jsonl",
        "--mqtt-outbox",
        "/var/lib/mine/mqtt.jsonl",
        "--can-trace",
        "/var/log/mine/can.log",
        "--status",
        "/var/log/mine/status.json",
        "--topic",
        "truck/001",
        "--can-id",
        "0x456"
    };
    a53_cli_options_t options;

    TEST_ASSERT_EQ_INT(0, a53_cli_parse(13, argv, &options));
    TEST_ASSERT_EQ_STR("/mnt/sdcard/samples.jsonl", options.storage_path);
    TEST_ASSERT_EQ_STR("/var/lib/mine/mqtt.jsonl", options.mqtt_outbox_path);
    TEST_ASSERT_EQ_STR("/var/log/mine/can.log", options.can_trace_path);
    TEST_ASSERT_EQ_STR("/var/log/mine/status.json", options.status_path);
    TEST_ASSERT_EQ_STR("truck/001", options.mqtt_topic);
    TEST_ASSERT_EQ_INT(0x456, options.can_id);
    return 0;
}

static int cli_loads_beginner_config_file(void)
{
    const char *config_path = "test-a53-demo.conf";
    const char *argv[] = {
        "okmx8mm-a53-demo",
        "--config",
        config_path
    };
    a53_cli_options_t options;

    remove(config_path);
    TEST_ASSERT_EQ_INT(0, write_text(config_path,
        "# beginner board config\n"
        "cycles=4\n"
        "file=examples/m4-input.csv\n"
        "storage=/mnt/sdcard/samples.jsonl\n"
        "mqtt_outbox=/mnt/sdcard/mqtt-outbox.jsonl\n"
        "can_trace=/mnt/sdcard/can-trace.log\n"
        "status=/mnt/sdcard/status.json\n"
        "topic=truck/001\n"
        "can_id=0x456\n"));

    TEST_ASSERT_EQ_INT(0, a53_cli_parse(3, argv, &options));
    TEST_ASSERT_EQ_INT(A53_SOURCE_FILE, options.source_kind);
    TEST_ASSERT_EQ_INT(4, options.cycles);
    TEST_ASSERT_EQ_STR("examples/m4-input.csv", options.source_path);
    TEST_ASSERT_EQ_STR("/mnt/sdcard/samples.jsonl", options.storage_path);
    TEST_ASSERT_EQ_STR("/mnt/sdcard/mqtt-outbox.jsonl", options.mqtt_outbox_path);
    TEST_ASSERT_EQ_STR("/mnt/sdcard/can-trace.log", options.can_trace_path);
    TEST_ASSERT_EQ_STR("/mnt/sdcard/status.json", options.status_path);
    TEST_ASSERT_EQ_STR("truck/001", options.mqtt_topic);
    TEST_ASSERT_EQ_INT(0x456, options.can_id);

    remove(config_path);
    return 0;
}

static int cli_rejects_unknown_config_key(void)
{
    const char *config_path = "test-bad-a53-demo.conf";
    const char *argv[] = {
        "okmx8mm-a53-demo",
        "--config",
        config_path
    };
    a53_cli_options_t options;

    remove(config_path);
    TEST_ASSERT_EQ_INT(0, write_text(config_path, "unknown=value\n"));
    TEST_ASSERT_EQ_INT(-1, a53_cli_parse(3, argv, &options));
    remove(config_path);
    return 0;
}

static int cli_rejects_file_without_path(void)
{
    const char *argv[] = {"okmx8mm-a53-demo", "--file"};
    a53_cli_options_t options;

    TEST_ASSERT_EQ_INT(-1, a53_cli_parse(2, argv, &options));
    return 0;
}

int main(void)
{
    TEST_RUN(cli_defaults_to_replay_source);
    TEST_RUN(cli_accepts_storage_cursor_check_mode);
    TEST_RUN(cli_accepts_storage_tail_recover_mode);
    TEST_RUN(cli_accepts_file_source_and_cycle_count);
    TEST_RUN(cli_accepts_output_settings);
    TEST_RUN(cli_loads_beginner_config_file);
    TEST_RUN(cli_rejects_unknown_config_key);
    TEST_RUN(cli_rejects_file_without_path);
    return 0;
}
