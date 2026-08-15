#include "demo1_console.h"

#include <stdio.h>

size_t demo1_format_batch_status(const demo1_batch_t *batch,
                                 char *buffer,
                                 size_t capacity) {
    int written;

    if (batch == NULL || buffer == NULL || capacity == 0U) {
        return 0U;
    }

    written = snprintf(buffer, capacity,
                       "demo1 seq=%lu ticks=%u start_us=%llu valid=0x%04X "
                       "hall=[%u,%u] oil=[%ld,%ld]\n",
                       (unsigned long)batch->sequence,
                       (unsigned)batch->sample_count,
                       (unsigned long long)batch->start_timestamp_us,
                       (unsigned)batch->aggregate_valid_mask,
                       (unsigned)batch->latest_hall_pulse_delta[0],
                       (unsigned)batch->latest_hall_pulse_delta[1],
                       (long)batch->latest_oil_raw[0],
                       (long)batch->latest_oil_raw[1]);
    if (written < 0) {
        return 0U;
    }
    if ((size_t)written >= capacity) {
        return capacity - 1U;
    }
    return (size_t)written;
}
