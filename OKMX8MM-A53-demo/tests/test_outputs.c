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

static int write_text(const char *path, const char *text)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return -1;
    }
    fputs(text, file);
    return fclose(file);
}

static long file_size(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    if (file == NULL) {
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }
    size = ftell(file);
    fclose(file);
    return size;
}

static int writers_create_beginner_readable_output_files(void)
{
    const char *storage_path = "test-storage.jsonl";
    const char *storage_cursor_path = "test-storage.jsonl.cursor";
    const char *mqtt_path = "test-mqtt-outbox.jsonl";
    const char *can_path = "test-can-trace.log";
    char text[2048];
    a53_m4_source_t source;
    a53_m4_batch_t batch;
    a53_m4_batch_t second_batch;
    long first_size;
    long second_size;
    long line_bytes = 0;

    remove(storage_path);
    remove(storage_cursor_path);
    remove(mqtt_path);
    remove(can_path);

    TEST_ASSERT_EQ_INT(0, a53_m4_replay_open(&source));
    TEST_ASSERT_EQ_INT(0, a53_m4_source_read(&source, &batch));
    TEST_ASSERT_EQ_INT(0, a53_m4_source_read(&source, &second_batch));
    a53_m4_source_close(&source);

    TEST_ASSERT_EQ_INT(0, a53_storage_append_batch(storage_path, &batch));
    TEST_ASSERT_EQ_INT(0, read_file(storage_path, text, sizeof(text)));
    TEST_ASSERT_TRUE(strstr(text, "\"sequence\":0") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "\"ai0\":1000") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "\"di_bits\":1") != NULL);
    TEST_ASSERT_EQ_INT(0, read_file(storage_cursor_path, text, sizeof(text)));
    TEST_ASSERT_TRUE(strstr(text, "file=test-storage.jsonl") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "sequence=0") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "byte_offset=") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "line_bytes=") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "line_checksum=") != NULL);
    first_size = file_size(storage_path);
    TEST_ASSERT_TRUE(first_size > 0);
    TEST_ASSERT_EQ_INT(0, a53_storage_append_batch(storage_path, &second_batch));
    second_size = file_size(storage_path);
    TEST_ASSERT_TRUE(second_size > first_size);
    TEST_ASSERT_EQ_INT(0, read_file(storage_cursor_path, text, sizeof(text)));
    TEST_ASSERT_TRUE(strstr(text, "sequence=1") != NULL);
    TEST_ASSERT_TRUE(sscanf(strstr(text, "line_bytes="), "line_bytes=%ld", &line_bytes) == 1);
    TEST_ASSERT_EQ_INT(second_size - first_size, line_bytes);
    TEST_ASSERT_TRUE(strstr(text, "line_checksum=") != NULL);
    TEST_ASSERT_EQ_INT(0, a53_storage_validate_cursor(storage_path));
    TEST_ASSERT_EQ_INT(0, write_text(storage_cursor_path,
        "file=test-storage.jsonl\nsequence=1\nbyte_offset=1\nline_bytes=1\nline_checksum=00000000\n"));
    TEST_ASSERT_EQ_INT(-1, a53_storage_validate_cursor(storage_path));

    TEST_ASSERT_EQ_INT(0, a53_mqtt_outbox_append(mqtt_path, "mine-truck/demo1", &batch));
    TEST_ASSERT_EQ_INT(0, read_file(mqtt_path, text, sizeof(text)));
    TEST_ASSERT_TRUE(strstr(text, "\"topic\":\"mine-truck/demo1\"") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "\"sequence\":0") != NULL);

    TEST_ASSERT_EQ_INT(0, a53_can_trace_append(can_path, 0x321u, &batch));
    TEST_ASSERT_EQ_INT(0, read_file(can_path, text, sizeof(text)));
    TEST_ASSERT_TRUE(strstr(text, "CAN id=0x321 seq=0") != NULL);
    TEST_ASSERT_TRUE(strstr(text, "ai0=1000") != NULL);

    remove(storage_path);
    remove(storage_cursor_path);
    remove(mqtt_path);
    remove(can_path);
    return 0;
}

int main(void)
{
    TEST_RUN(writers_create_beginner_readable_output_files);
    return 0;
}
