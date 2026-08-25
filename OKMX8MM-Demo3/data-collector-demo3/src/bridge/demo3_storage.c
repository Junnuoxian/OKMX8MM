#include "demo3_storage.h"

int demo3_store_sample(file_storage_context_t *storage,
                       const demo3_sample_t *sample)
{
    if (storage == 0 || sample == 0) {
        return -1;
    }
    return file_storage_write(storage,
                              (char *)sample,
                              (int)sizeof(*sample));
}
