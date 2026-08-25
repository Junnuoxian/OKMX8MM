#include "demo3_rpmsg_reader.h"

#include "demo3_rpmsg.h"

int demo3_rpmsg_reader_step(const demo3_rpmsg_reader_t *reader,
                            void *sink_context,
                            demo3_sample_sink_fn sink)
{
    uint8_t frame[DEMO3_RPMSG_FRAME_LENGTH];
    int frame_length;

    if (reader == 0 || reader->read == 0 || sink == 0) {
        return -1;
    }
    frame_length = reader->read(reader->context, frame, sizeof(frame));
    if (frame_length <= 0) {
        return frame_length;
    }
    if ((size_t)frame_length != sizeof(frame)) {
        return -2;
    }
    return demo3_rpmsg_process_frame(frame,
                                     (size_t)frame_length,
                                     sink_context,
                                     sink);
}
