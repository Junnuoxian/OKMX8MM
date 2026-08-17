#include <stdio.h>

#include "demo1_console.h"
#include "demo1_engine.h"
#include "demo1_mock_source.h"

int main(void) {
    demo1_mock_source_t source;
    demo1_engine_t engine;
    demo1_batch_t batch;
    char line[160];

    if (demo1_mock_source_init(&source, 20260804U) != 0) {
        return 1;
    }
    if (demo1_engine_init(&engine, &source) != 0) {
        return 1;
    }

    for (uint32_t tick = 0U; tick < 30U; ++tick) {
        if (demo1_engine_tick(&engine, (uint64_t)tick * DEMO1_TICK_PERIOD_US) &&
            demo1_engine_consume_batch(&engine, &batch) == 0) {
            if (demo1_format_batch_status(&batch, line, sizeof(line)) == 0U) {
                return 1;
            }
            fputs(line, stdout);
        }
    }
    return 0;
}
