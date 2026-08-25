#include "demo3_rpmsg_storage.h"

#include "demo3_rpmsg_bridge.h"
#include "demo3_storage.h"

static int store_sample(void *context, const demo3_sample_t *sample)
{
    return demo3_store_sample((file_storage_context_t *)context, sample);
}

int demo3_rpmsg_store_frame(const uint8_t *frame,
                            size_t frame_length,
                            file_storage_context_t *storage)
{
    return demo3_rpmsg_process_frame(frame,
                                     frame_length,
                                     storage,
                                     store_sample);
}
