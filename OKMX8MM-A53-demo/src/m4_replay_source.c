#include "a53_demo.h"

#include <stddef.h>
#include <string.h>

int a53_m4_replay_open(a53_m4_source_t *source)
{
    if (source == NULL) {
        return -1;
    }
    source->next_sequence = 0;
    return 0;
}

int a53_m4_source_read(a53_m4_source_t *source, a53_m4_batch_t *batch)
{
    uint32_t sequence;
    int sample;
    int channel;

    if (source == NULL || batch == NULL) {
        return -1;
    }

    sequence = source->next_sequence++;
    memset(batch, 0, sizeof(*batch));
    batch->sequence = sequence;
    batch->start_timestamp_us = (uint64_t)sequence * 5000u;
    batch->sample_rate_hz = A53_SAMPLE_RATE_HZ;
    batch->sample_count = A53_BATCH_SAMPLE_COUNT;
    batch->analog_channel_count = A53_ANALOG_CHANNEL_COUNT;
    batch->aggregate_valid_mask = 0x03ffu;
    batch->speed_pulse_delta = (uint16_t)(11u + sequence);
    batch->speed_period_us = 50000u;

    for (sample = 0; sample < A53_BATCH_SAMPLE_COUNT; sample++) {
        batch->digital_states[sample] = (sample % 2 == 0) ? 0x01u : 0x00u;
        for (channel = 0; channel < A53_ANALOG_CHANNEL_COUNT; channel++) {
            batch->analog_samples[sample][channel] =
                (int16_t)(1000 + (int)sequence * 100 + sample * 10 + channel);
        }
    }

    return 0;
}

void a53_m4_source_close(a53_m4_source_t *source)
{
    if (source != NULL) {
        source->next_sequence = 0;
    }
}
