#ifndef DEMO1_BOARD_HOOKS_H
#define DEMO1_BOARD_HOOKS_H

#include <stddef.h>
#include <stdint.h>

void demo1_board_init(void);
void demo1_board_wait_next_tick(void);
uint64_t demo1_board_now_us(void);
void demo1_board_uart_write(const char *buffer, size_t length);
void *demo1_board_rs485_context(void);
int demo1_board_rs485_write_bytes(void *context, const uint8_t *data, size_t length);
int demo1_board_rs485_read_byte(void *context, uint8_t *out_byte);
uint64_t demo1_board_rs485_now_us(void *context);

#endif
