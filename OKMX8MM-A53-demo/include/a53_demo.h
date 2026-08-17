#ifndef A53_DEMO_H
#define A53_DEMO_H

#include <stdint.h>

enum {
    A53_ANALOG_CHANNEL_COUNT = 10,
    A53_BATCH_SAMPLE_COUNT = 10,
    A53_SAMPLE_RATE_HZ = 2000
};

typedef struct {
    uint32_t sequence;
    uint64_t start_timestamp_us;
    uint32_t sample_rate_hz;
    uint8_t sample_count;
    uint8_t analog_channel_count;
    uint16_t aggregate_valid_mask;
    int16_t analog_samples[A53_BATCH_SAMPLE_COUNT][A53_ANALOG_CHANNEL_COUNT];
    uint8_t digital_states[A53_BATCH_SAMPLE_COUNT];
    uint16_t speed_pulse_delta;
    uint32_t speed_period_us;
} a53_m4_batch_t;

typedef struct {
    uint32_t next_sequence;
} a53_m4_source_t;

typedef struct {
    const char *storage_path;
    const char *mqtt_outbox_path;
    const char *can_trace_path;
    const char *mqtt_topic;
    uint32_t can_id;
    uint32_t cycles;
} a53_pipeline_config_t;

int a53_m4_replay_open(a53_m4_source_t *source);
int a53_m4_source_read(a53_m4_source_t *source, a53_m4_batch_t *batch);
void a53_m4_source_close(a53_m4_source_t *source);

int a53_storage_append_batch(const char *path, const a53_m4_batch_t *batch);
int a53_mqtt_outbox_append(const char *path, const char *topic, const a53_m4_batch_t *batch);
int a53_can_trace_append(const char *path, uint32_t can_id, const a53_m4_batch_t *batch);
int a53_pipeline_run(const a53_pipeline_config_t *config);

#endif
