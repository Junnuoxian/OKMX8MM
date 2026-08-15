#include "demo1_board_hooks.h"
#include "demo1_console.h"
#include "demo1_engine.h"
#include "demo1_rs485_transport.h"
#include "demo1_wheel_board_source.h"

enum {
    DEMO1_M4_RS485_RESPONSE_TIMEOUT_US = 100000
};

int main(void) {
    demo1_rs485_transport_t transport;
    demo1_wheel_board_source_t source;
    demo1_engine_t engine;
    demo1_batch_t batch;
    char line[160];

    demo1_board_init();
    if (demo1_rs485_transport_init(&transport,
                                   demo1_board_rs485_context(),
                                   demo1_board_rs485_write_bytes,
                                   demo1_board_rs485_read_byte,
                                   demo1_board_rs485_now_us,
                                   DEMO1_M4_RS485_RESPONSE_TIMEOUT_US) != 0) {
        return 1;
    }
    if (demo1_wheel_board_source_init(&source,
                                      &transport,
                                      demo1_rs485_transport_write,
                                      demo1_rs485_transport_read,
                                      1U,
                                      0U) != 0) {
        return 1;
    }
    if (demo1_engine_init_with_source(&engine,
                                      &source,
                                      demo1_wheel_board_source_read_tick) != 0) {
        return 1;
    }

    for (;;) {
        demo1_board_wait_next_tick();
        if (demo1_engine_tick(&engine, demo1_board_now_us()) &&
            demo1_engine_consume_batch(&engine, &batch) == 0) {
            size_t length = demo1_format_batch_status(&batch, line, sizeof(line));
            if (length > 0U) {
                demo1_board_uart_write(line, length);
            }
        }
    }
}
