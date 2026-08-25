#include "demo3_rpmsg_bridge.h"

#include "demo3_m4_rpmsg.h"

int demo3_rpmsg_process_frame(const uint8_t *frame,
                              size_t frame_length,
                              void *context,
                              demo3_sample_sink_fn sink)
{
    demo3_sample_t sample;
    int result;

    if (sink == 0) {
        return -1;
    }
    result = demo3_m4_rpmsg_decode(frame, frame_length, &sample);
    if (result != 0) {
        return result;
    }
    return sink(context, &sample);
}
