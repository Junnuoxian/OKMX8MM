#ifndef DEMO1_ENGINE_H
#define DEMO1_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "demo1_mock_source.h"
#include "demo1_types.h"

typedef int (*demo1_sample_read_fn)(void *context,
                                    uint64_t timestamp_us,
                                    demo1_tick_sample_t *out);

typedef struct {
    void *source_context;
    demo1_sample_read_fn read_tick;
    demo1_batch_t current_batch;
    demo1_batch_t ready_batch;
    uint8_t sample_index;
    uint32_t next_sequence;
    bool batch_ready;
} demo1_engine_t;

int demo1_engine_init(demo1_engine_t *engine, demo1_mock_source_t *source);
int demo1_engine_init_with_source(demo1_engine_t *engine,
                                  void *source_context,
                                  demo1_sample_read_fn read_tick);
bool demo1_engine_tick(demo1_engine_t *engine, uint64_t timestamp_us);
int demo1_engine_consume_batch(demo1_engine_t *engine, demo1_batch_t *out);

#endif
