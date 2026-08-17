#include "m4_source_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int a53_m4_file_open(a53_m4_source_t *source, const char *path)
{
    if (source == 0 || path == 0) {
        return -1;
    }

    memset(source, 0, sizeof(*source));
    source->kind = A53_SOURCE_FILE;
    source->file = fopen(path, "rb");
    if (source->file == 0) {
        return -1;
    }

    return 0;
}

int a53_m4_file_read_next(a53_m4_source_t *source, a53_m4_batch_t *batch)
{
    char line[256];
    char *cursor;
    char *token;
    long values[14];
    int index;
    int sample;
    int channel;

    if (source == 0 || source->file == 0 || batch == 0) {
        return -1;
    }

    if (fgets(line, sizeof(line), source->file) == 0) {
        return -1;
    }

    cursor = line;
    for (index = 0; index < 14; index++) {
        token = strtok(index == 0 ? cursor : 0, ",\r\n");
        if (token == 0) {
            return -1;
        }
        values[index] = strtol(token, 0, 10);
    }

    memset(batch, 0, sizeof(*batch));
    batch->sequence = (uint32_t)values[0];
    batch->start_timestamp_us = (uint64_t)batch->sequence * 5000u;
    batch->sample_rate_hz = A53_SAMPLE_RATE_HZ;
    batch->sample_count = A53_BATCH_SAMPLE_COUNT;
    batch->analog_channel_count = A53_ANALOG_CHANNEL_COUNT;
    batch->aggregate_valid_mask = 0x03ffu;
    batch->speed_pulse_delta = (uint16_t)values[12];
    batch->speed_period_us = (uint32_t)values[13];

    for (sample = 0; sample < A53_BATCH_SAMPLE_COUNT; sample++) {
        batch->digital_states[sample] = (uint8_t)values[11];
        for (channel = 0; channel < A53_ANALOG_CHANNEL_COUNT; channel++) {
            batch->analog_samples[sample][channel] = (int16_t)values[channel + 1];
        }
    }

    return 0;
}
