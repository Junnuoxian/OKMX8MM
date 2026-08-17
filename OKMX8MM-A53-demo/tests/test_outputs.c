#include "a53_demo.h"
#include "test_harness.h"

#include <stdio.h>

static int read_file(const char *path, char *buffer, size_t size)
{
    FILE *file = fopen(path, "rb");
    size_t used;
    if (file == NULL) {
        return -1;
    }
    used = fread(buffer, 1, size - 1, file);
    buffer[used] = '\0';
    fclose(file);
    return 0;
}

static int writers_create_beginner_readable_output_files(void)
{
    const char *storage_path = "test-storage.jsonl";
    const char *mqtt_path = "test-mqtt-outbox.jsonl";
    const char *can_path = "test-can-trace.log";
    char text[2048];
    a53_m4_source_t source;
    a53_m4_batch_t batch;

    remove(storage_path);
    remove(mqtt_path);
    remove(can_path);

    TEST_ASSERT_EQ_INT(0, a53_m4_replay_open(&source));
    TEST_ASSERT_EQ_INT(0, a53_m4_source_read(&source, &batch));
    a53_m4_source_close(&source);

    TEST_ASSERT_EQ_INT(0, a53_storage_append_batch(storage_path, &batch));
    TEST_ASSERT_EQ_INT(0, read_file(storage_path, text, sizeof(text)));
    TEST_ASSERT_TRUE(strstr(text, "\"sequence\":0") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "\"ai0\":1000") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "\"di_bits\":1") != NULL);

    TEST_ASSERT_EQ_INT(0, a53_mqtt_outbox_append(mqtt_path, "mine-truck/demo1", &batch));
    TEST_ASSERT_EQ_INT(0, read_file(mqtt_path, text, sizeof(text)));
    TEST_ASSERT_TRUE(strstr(text, "\"topic\":\"mine-truck/demo1\"") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "\"sequence\":0") != NULL);

    TEST_ASSERT_EQ_INT(0, a53_can_trace_append(can_path, 0x321u, &batch));
    TEST_ASSERT_EQ_INT(0, read_file(can_path, text, sizeof(text)));
    TEST_ASSERT_TRUE(strstr(text, "CAN id=0x321 seq=0") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "ai0=1000") != NULL);

    remove(storage_path);
    remove(mqtt_path);
    remove(can_path);
    return 0;
}

int main(void)
{
    TEST_RUN(writers_create_beginner_readable_output_files);
    return 0;
}
