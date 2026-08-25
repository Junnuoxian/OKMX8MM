#ifndef DEMO3_STORAGE_H
#define DEMO3_STORAGE_H

#include "demo3_protocol.h"
#include "file-storage/file-storage.h"

int demo3_store_sample(file_storage_context_t *storage,
                       const demo3_sample_t *sample);

#endif
