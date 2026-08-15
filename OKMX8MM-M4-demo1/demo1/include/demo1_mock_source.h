#ifndef DEMO1_MOCK_SOURCE_H
#define DEMO1_MOCK_SOURCE_H

#include <stdint.h>

#include "demo1_types.h"

typedef struct {
    uint32_t seed;
    uint32_t tick_count;
    int initialized;
} demo1_mock_source_t;

int demo1_mock_source_init(demo1_mock_source_t *source, uint32_t seed);
int demo1_mock_source_read_tick(demo1_mock_source_t *source,
                                uint64_t timestamp_us,
                                demo1_tick_sample_t *out);

#endif
