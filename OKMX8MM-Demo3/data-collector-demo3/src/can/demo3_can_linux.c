#include "demo3_can_linux.h"

#include <errno.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "demo3_can.h"

int demo3_can_linux_open(demo3_can_linux_endpoint_t *endpoint,
                         const char *interface_name,
                         uint32_t base_id)
{
    struct ifreq interface_request;
    struct sockaddr_can can_address;

    if (endpoint == 0 || interface_name == 0 || interface_name[0] == '\0') {
        return -1;
    }
    endpoint->fd = -1;
    endpoint->fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (endpoint->fd < 0) {
        return -2;
    }
    memset(&interface_request, 0, sizeof(interface_request));
    strncpy(interface_request.ifr_name, interface_name,
            sizeof(interface_request.ifr_name) - 1u);
    if (ioctl(endpoint->fd, SIOCGIFINDEX, &interface_request) < 0) {
        close(endpoint->fd);
        endpoint->fd = -1;
        return -3;
    }
    memset(&can_address, 0, sizeof(can_address));
    can_address.can_family = AF_CAN;
    can_address.can_ifindex = interface_request.ifr_ifindex;
    if (bind(endpoint->fd, (struct sockaddr *)&can_address,
             sizeof(can_address)) < 0) {
        close(endpoint->fd);
        endpoint->fd = -1;
        return -4;
    }
    endpoint->base_id = base_id;
    return 0;
}

int demo3_can_linux_send_sample(demo3_can_linux_endpoint_t *endpoint,
                                const demo3_sample_t *sample)
{
    demo3_can_frame_t encoded;
    struct can_frame frame;
    uint8_t frame_index;

    if (endpoint == 0 || sample == 0 || endpoint->fd < 0) {
        return -1;
    }
    for (frame_index = 0u; frame_index < DEMO3_CAN_FRAME_COUNT; ++frame_index) {
        if (demo3_can_encode_sample(sample, endpoint->base_id,
                                    frame_index, &encoded) != 0) {
            return -2;
        }
        memset(&frame, 0, sizeof(frame));
        frame.can_id = encoded.id;
        frame.can_dlc = encoded.dlc;
        memcpy(frame.data, encoded.data, sizeof(frame.data));
        if (write(endpoint->fd, &frame, sizeof(frame)) != (ssize_t)sizeof(frame)) {
            return errno == EINTR ? -3 : -4;
        }
    }
    return 0;
}

int demo3_can_linux_close(demo3_can_linux_endpoint_t *endpoint)
{
    int result;

    if (endpoint == 0 || endpoint->fd < 0) {
        return -1;
    }
    result = close(endpoint->fd);
    endpoint->fd = -1;
    return result == 0 ? 0 : -2;
}
