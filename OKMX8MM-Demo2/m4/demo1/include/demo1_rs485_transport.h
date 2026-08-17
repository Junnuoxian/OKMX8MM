#ifndef DEMO1_RS485_TRANSPORT_H
#define DEMO1_RS485_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

typedef int (*demo1_rs485_write_bytes_fn)(void *context,
                                          const uint8_t *data,
                                          size_t length);
typedef int (*demo1_rs485_read_byte_fn)(void *context, uint8_t *out_byte);
typedef uint64_t (*demo1_rs485_now_us_fn)(void *context);

typedef struct {
    void *port_context;
    demo1_rs485_write_bytes_fn write_bytes;
    demo1_rs485_read_byte_fn read_byte;
    demo1_rs485_now_us_fn now_us;
    uint32_t response_timeout_us;
} demo1_rs485_transport_t;

int demo1_rs485_transport_init(demo1_rs485_transport_t *transport,
                               void *port_context,
                               demo1_rs485_write_bytes_fn write_bytes,
                               demo1_rs485_read_byte_fn read_byte,
                               demo1_rs485_now_us_fn now_us,
                               uint32_t response_timeout_us);
int demo1_rs485_transport_write(void *context, const uint8_t *data, size_t length);
int demo1_rs485_transport_read(void *context,
                               uint8_t *data,
                               size_t capacity,
                               size_t *out_length);

#endif
