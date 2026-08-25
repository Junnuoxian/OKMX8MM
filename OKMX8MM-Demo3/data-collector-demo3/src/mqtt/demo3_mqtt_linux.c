#include "demo3_mqtt_linux.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int send_all(int fd, const uint8_t *data, size_t length)
{
    size_t sent = 0u;

    while (sent < length) {
        ssize_t result = send(fd, data + sent, length - sent, 0);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (result == 0) {
            return -1;
        }
        sent += (size_t)result;
    }
    return 0;
}

static int encode_remaining_length(uint8_t *buffer,
                                   size_t capacity,
                                   size_t value,
                                   size_t *length)
{
    size_t index = 0u;

    do {
        uint8_t digit;
        if (index >= capacity || index >= 4u) {
            return -1;
        }
        digit = (uint8_t)(value % 128u);
        value /= 128u;
        if (value != 0u) {
            digit |= 0x80u;
        }
        buffer[index++] = digit;
    } while (value != 0u);
    *length = index;
    return 0;
}

static void put_u16_be(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value >> 8u);
    buffer[1] = (uint8_t)(value & 0xFFu);
}

static int mqtt_connect(int fd, const char *client_id)
{
    uint8_t packet[512];
    size_t client_length;
    size_t remaining_length;
    size_t encoded_length;
    size_t offset = 0u;
    uint8_t response[4];

    client_length = strlen(client_id);
    if (client_length == 0u || client_length > 255u) {
        return -1;
    }
    remaining_length = 10u + 2u + client_length;
    packet[offset++] = 0x10u;
    if (encode_remaining_length(packet + offset,
                                sizeof(packet) - offset,
                                remaining_length,
                                &encoded_length) != 0) {
        return -1;
    }
    offset += encoded_length;
    put_u16_be(packet + offset, 4u);
    offset += 2u;
    memcpy(packet + offset, "MQTT", 4u);
    offset += 4u;
    packet[offset++] = 4u;
    packet[offset++] = 0x02u;
    put_u16_be(packet + offset, 60u);
    offset += 2u;
    put_u16_be(packet + offset, (uint16_t)client_length);
    offset += 2u;
    memcpy(packet + offset, client_id, client_length);
    offset += client_length;
    if (send_all(fd, packet, offset) != 0 ||
        recv(fd, response, sizeof(response), MSG_WAITALL) != (ssize_t)sizeof(response)) {
        return -1;
    }
    if (response[0] != 0x20u || response[1] != 0x02u || response[3] != 0u) {
        return -1;
    }
    return 0;
}

int demo3_mqtt_linux_open(demo3_mqtt_linux_endpoint_t *endpoint,
                          const char *broker_host,
                          int broker_port,
                          const char *client_id)
{
    struct addrinfo hints;
    struct addrinfo *results = 0;
    struct addrinfo *entry;
    char port_text[16];
    int fd = -1;

    if (endpoint == 0 || broker_host == 0 || client_id == 0 ||
        broker_host[0] == '\0' || client_id[0] == '\0' ||
        broker_port <= 0 || broker_port > 65535) {
        return -1;
    }
    endpoint->fd = -1;
    (void)snprintf(port_text, sizeof(port_text), "%d", broker_port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(broker_host, port_text, &hints, &results) != 0) {
        return -2;
    }
    for (entry = results; entry != 0; entry = entry->ai_next) {
        fd = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (connect(fd, entry->ai_addr, entry->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(results);
    if (fd < 0 || mqtt_connect(fd, client_id) != 0) {
        if (fd >= 0) {
            close(fd);
        }
        return -3;
    }
    endpoint->fd = fd;
    return 0;
}

int demo3_mqtt_linux_publish(demo3_mqtt_linux_endpoint_t *endpoint,
                             const char *topic,
                             const char *payload,
                             size_t payload_length)
{
    uint8_t header[5];
    size_t topic_length;
    size_t remaining_length;
    size_t encoded_length;
    size_t offset = 0u;

    if (endpoint == 0 || endpoint->fd < 0 || topic == 0 || payload == 0) {
        return -1;
    }
    topic_length = strlen(topic);
    if (topic_length == 0u || topic_length > 65535u) {
        return -1;
    }
    remaining_length = 2u + topic_length + payload_length;
    header[offset++] = 0x30u;
    if (encode_remaining_length(header + offset,
                                sizeof(header) - offset,
                                remaining_length,
                                &encoded_length) != 0) {
        return -2;
    }
    offset += encoded_length;
    if (send_all(endpoint->fd, header, offset) != 0) {
        return -3;
    }
    {
        uint8_t topic_length_bytes[2];
        put_u16_be(topic_length_bytes, (uint16_t)topic_length);
        if (send_all(endpoint->fd, topic_length_bytes, sizeof(topic_length_bytes)) != 0 ||
            send_all(endpoint->fd, (const uint8_t *)topic, topic_length) != 0 ||
            send_all(endpoint->fd, (const uint8_t *)payload, payload_length) != 0) {
            return -3;
        }
    }
    return 0;
}

int demo3_mqtt_linux_close(demo3_mqtt_linux_endpoint_t *endpoint)
{
    int result;

    if (endpoint == 0 || endpoint->fd < 0) {
        return -1;
    }
    result = close(endpoint->fd);
    endpoint->fd = -1;
    return result == 0 ? 0 : -2;
}
