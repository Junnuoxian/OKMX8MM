#ifndef DEMO3_RPMSG_STORAGE_H
#define DEMO3_RPMSG_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "file-storage/file-storage.h"

int demo3_rpmsg_store_frame(const uint8_t *frame,
                            size_t frame_length,
                            file_storage_context_t *storage);

#endif
