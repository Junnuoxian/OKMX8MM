#include "a53_demo.h"
#include "test_harness.h"

static int cli_defaults_to_replay_source(void)
{
    const char *argv[] = {"okmx8mm-a53-demo"};
    a53_cli_options_t options;

    TEST_ASSERT_EQ_INT(0, a53_cli_parse(1, argv, &options));
    TEST_ASSERT_EQ_INT(A53_SOURCE_REPLAY, options.source_kind);
    TEST_ASSERT_EQ_INT(5, options.cycles);
    TEST_ASSERT_TRUE(options.source_path == 0);
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
    TEST_RUN(cli_accepts_file_source_and_cycle_count);
    TEST_RUN(cli_rejects_file_without_path);
    return 0;
}
