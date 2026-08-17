#include "a53_demo.h"

int a53_pipeline_run(const a53_pipeline_config_t *config)
{
    a53_m4_source_t source;
    a53_m4_batch_t batch;
    uint32_t index;
    uint32_t expected_sequence;
    int has_expected_sequence = 0;

    if (config == 0 ||
        config->storage_path == 0 ||
        config->mqtt_outbox_path == 0 ||
        config->can_trace_path == 0 ||
        config->mqtt_topic == 0) {
        return -1;
    }

    if (config->source_kind == A53_SOURCE_FILE) {
        if (a53_m4_file_open(&source, config->source_path) != 0) {
            return -1;
        }
    } else if (a53_m4_replay_open(&source) != 0) {
        return -1;
    }

    for (index = 0; index < config->cycles; index++) {
        if (a53_m4_source_read(&source, &batch) != 0) {
            a53_m4_source_close(&source);
            return -1;
        }
        if (has_expected_sequence && batch.sequence != expected_sequence) {
            a53_m4_source_close(&source);
            return -1;
        }
        has_expected_sequence = 1;
        expected_sequence = batch.sequence + 1u;

        if (a53_storage_append_batch(config->storage_path, &batch) != 0 ||
            a53_mqtt_outbox_append(config->mqtt_outbox_path, config->mqtt_topic, &batch) != 0 ||
            a53_can_trace_append(config->can_trace_path, config->can_id, &batch) != 0) {
            a53_m4_source_close(&source);
            return -1;
        }
    }

    a53_m4_source_close(&source);
    return 0;
}
