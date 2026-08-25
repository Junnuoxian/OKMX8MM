#include "demo3_rpmsg_linux.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include "demo3_rpmsg.h"

int demo3_rpmsg_linux_open(demo3_rpmsg_linux_endpoint_t *endpoint,
                           const char *device_path,
                           int poll_timeout_ms)
{
    if (endpoint == 0 || device_path == 0 || device_path[0] == '\0') {
        return -1;
    }
    endpoint->fd = open(device_path, O_RDWR | O_CLOEXEC);
    if (endpoint->fd < 0) {
        return -2;
    }
    endpoint->poll_timeout_ms = poll_timeout_ms < 0 ? -1 : poll_timeout_ms;
    return 0;
}

int demo3_rpmsg_linux_read(void *context,
                           uint8_t *frame,
                           size_t capacity)
{
    demo3_rpmsg_linux_endpoint_t *endpoint =
        (demo3_rpmsg_linux_endpoint_t *)context;
    struct pollfd descriptor;
    ssize_t bytes_read;
    int poll_result;

    if (endpoint == 0 || frame == 0 ||
        capacity < DEMO3_RPMSG_FRAME_LENGTH || endpoint->fd < 0) {
        return -1;
    }
    descriptor.fd = endpoint->fd;
    descriptor.events = POLLIN;
    descriptor.revents = 0;
    poll_result = poll(&descriptor, 1, endpoint->poll_timeout_ms);
    if (poll_result == 0) {
        return 0;
    }
    if (poll_result < 0) {
        return errno == EINTR ? 0 : -2;
    }
    if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        return -3;
    }
    bytes_read = read(endpoint->fd, frame, DEMO3_RPMSG_FRAME_LENGTH);
    if (bytes_read < 0) {
        return errno == EINTR ? 0 : -4;
    }
    if ((size_t)bytes_read != DEMO3_RPMSG_FRAME_LENGTH) {
        return -5;
    }
    return (int)bytes_read;
}

int demo3_rpmsg_linux_close(demo3_rpmsg_linux_endpoint_t *endpoint)
{
    int result;

    if (endpoint == 0 || endpoint->fd < 0) {
        return -1;
    }
    result = close(endpoint->fd);
    endpoint->fd = -1;
    return result == 0 ? 0 : -2;
}
