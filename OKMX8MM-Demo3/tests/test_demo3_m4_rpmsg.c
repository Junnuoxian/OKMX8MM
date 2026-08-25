#include <string.h>

#include "demo3_m4_rpmsg.h"

typedef struct {
    uint8_t frame[DEMO3_M4_RPMSG_FRAME_LENGTH];
    size_t length;
} mock_endpoint_t;

static int mock_send(void *context, const uint8_t *data, size_t length)
{
    mock_endpoint_t *mock = (mock_endpoint_t *)context;
    if (length != sizeof(mock->frame)) {
        return -1;
    }
    memcpy(mock->frame, data, length);
    mock->length = length;
    return 0;
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

int main(void)
{
    mock_endpoint_t mock = {0};
    demo3_m4_rpmsg_endpoint_t endpoint;
    demo3_sample_t sample = {0};
    uint16_t received_crc;

    endpoint.context = &mock;
    endpoint.send = mock_send;
    sample.sequence = 12u;
    sample.timestamp_ms = 99u;
    sample.analog[0] = -1234;
    sample.digital_bits = 5u;
    sample.speed_rpm = 88u;
    sample.flags = DEMO3_SAMPLE_VALID;

    if (demo3_m4_rpmsg_publish(&endpoint, &sample) != 0) {
        return 1;
    }
    if (mock.length != DEMO3_M4_RPMSG_FRAME_LENGTH ||
        mock.frame[0] != DEMO3_M4_RPMSG_MAGIC ||
        mock.frame[1] != DEMO3_PROTOCOL_VERSION ||
        mock.frame[2] != DEMO3_M4_RPMSG_SAMPLE_TYPE ||
        read_u32_le(mock.frame + 5u) != 12u ||
        read_u32_le(mock.frame + 9u) != 99u ||
        (int32_t)read_u32_le(mock.frame + 13u) != -1234) {
        return 2;
    }
    received_crc = (uint16_t)mock.frame[DEMO3_M4_RPMSG_FRAME_LENGTH - 2u] |
                   (uint16_t)((uint16_t)mock.frame[DEMO3_M4_RPMSG_FRAME_LENGTH - 1u] << 8u);
    if (demo3_crc16_modbus(mock.frame, DEMO3_M4_RPMSG_FRAME_LENGTH - 2u) != received_crc) {
        return 3;
    }
    return 0;
}
