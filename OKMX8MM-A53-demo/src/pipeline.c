#include "a53_demo.h"
#include "file_utils.h"

#include <stdio.h>

static int write_status(const a53_pipeline_config_t *config,
                        int ok,
                        uint32_t processed_batches,
                        uint32_t last_sequence,
                        const char *error,
                        uint32_t expected_sequence,
                        uint32_t actual_sequence)
{
    FILE *file;

    if (config == 0 || config->status_path == 0) {
        return 0;
    }

    file = a53_open_write_text(config->status_path);
    if (file == 0) {
        return -1;
    }

    if (ok) {
        fprintf(file,
            "{\"ok\":true,\"processed_batches\":%u,\"last_sequence\":%u}\n",
            processed_batches,
            last_sequence);
    } else {
        fprintf(file,
            "{\"ok\":false,\"processed_batches\":%u,\"last_sequence\":%u,"
            "\"error\":\"%s\",\"expected_sequence\":%u,\"actual_sequence\":%u}\n",
            processed_batches,
            last_sequence,
            error == 0 ? "unknown" : error,
            expected_sequence,
            actual_sequence);
    }

    return fclose(file);
}

int a53_pipeline_run(const a53_pipeline_config_t *config)
{
    a53_m4_source_t source;
    a53_m4_batch_t batch;
    uint32_t index;
    uint32_t expected_sequence = 0;
    uint32_t last_sequence = 0;
    uint32_t processed_batches = 0;
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
            write_status(config, 0, 0, 0, "source_open", 0, 0);
            return -1;
        }
    } else if (a53_m4_replay_open(&source) != 0) {
        write_status(config, 0, 0, 0, "source_open", 0, 0);
        return -1;
    }

    for (index = 0; index < config->cycles; index++) {
        if (a53_m4_source_read(&source, &batch) != 0) {
            a53_m4_source_close(&source);
            write_status(config, 0, processed_batches, last_sequence, "source_read", expected_sequence, 0);
            return -1;
        }
        if (has_expected_sequence && batch.sequence != expected_sequence) {
            a53_m4_source_close(&source);
            write_status(config,
                0,
                processed_batches,
                last_sequence,
                "sequence_gap",
                expected_sequence,
                batch.sequence);
            return -1;
        }
        has_expected_sequence = 1;
        expected_sequence = batch.sequence + 1u;

        if (a53_storage_append_batch(config->storage_path, &batch) != 0 ||
            a53_mqtt_outbox_append(config->mqtt_outbox_path, config->mqtt_topic, &batch) != 0 ||
            a53_can_trace_append(config->can_trace_path, config->can_id, &batch) != 0) {
            a53_m4_source_close(&source);
            write_status(config, 0, processed_batches, last_sequence, "output_write", expected_sequence, batch.sequence);
            return -1;
        }
        processed_batches++;
        last_sequence = batch.sequence;
    }

    a53_m4_source_close(&source);
    return write_status(config, 1, processed_batches, last_sequence, 0, 0, 0);
}
