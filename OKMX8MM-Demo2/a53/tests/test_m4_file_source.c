#include "a53_demo.h"
#include "test_harness.h"

#include <stdio.h>

static int write_text(const char *path, const char *text)
{
    FILE *file = fopen(path, "wb");
    if (file == 0) {
        return -1;
    }
    fputs(text, file);
    return fclose(file);
}

static int file_source_reads_one_realistic_m4_line(void)
{
    const char *path = "test-m4-input.csv";
    a53_m4_source_t source;
    a53_m4_batch_t batch;

    remove(path);
    TEST_ASSERT_EQ_INT(0, write_text(path, "7,201,202,203,204,205,206,207,208,209,210,3,41,61000\n"));

    TEST_ASSERT_EQ_INT(0, a53_m4_file_open(&source, path));
    TEST_ASSERT_EQ_INT(0, a53_m4_source_read(&source, &batch));
    a53_m4_source_close(&source);

    TEST_ASSERT_EQ_INT(7, batch.sequence);
    TEST_ASSERT_EQ_INT(10, batch.sample_count);
    TEST_ASSERT_EQ_INT(10, batch.analog_channel_count);
    TEST_ASSERT_EQ_INT(201, batch.analog_samples[0][0]);
    TEST_ASSERT_EQ_INT(210, batch.analog_samples[0][9]);
    TEST_ASSERT_EQ_INT(3, batch.digital_states[0]);
    TEST_ASSERT_EQ_INT(41, batch.speed_pulse_delta);
    TEST_ASSERT_EQ_INT(61000, batch.speed_period_us);

    remove(path);
    return 0;
}

int main(void)
{
    TEST_RUN(file_source_reads_one_realistic_m4_line);
    return 0;
}
