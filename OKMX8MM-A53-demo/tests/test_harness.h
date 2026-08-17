#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <string.h>

#define TEST_ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "%s:%d assertion failed: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } \
} while (0)

#define TEST_ASSERT_EQ_INT(expected, actual) \
    TEST_ASSERT_TRUE((long long)(expected) == (long long)(actual))

#define TEST_ASSERT_EQ_STR(expected, actual) \
    TEST_ASSERT_TRUE(strcmp((expected), (actual)) == 0)

#define TEST_RUN(fn) do { \
    int result = (fn)(); \
    if (result != 0) { \
        return result; \
    } \
} while (0)

#endif
