#include "demo1_board_hooks.h"

#include <stdbool.h>

#include "board.h"
#include "demo1_types.h"
#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_uart.h"
#include "pin_mux.h"

#ifndef DEMO1_RS485_UART_BAUDRATE
#define DEMO1_RS485_UART_BAUDRATE 9600U
#endif

#define DEMO1_RS485_UART UART3
#define DEMO1_RS485_UART_CLK_FREQ                                                                  \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / CLOCK_GetRootPreDivider(kCLOCK_RootUart3) /         \
     CLOCK_GetRootPostDivider(kCLOCK_RootUart3) / 10U)

static volatile uint32_t g_tick_count;

typedef struct {
    bool initialized;
} demo1_board_rs485_port_t;

static demo1_board_rs485_port_t g_rs485_port;

void SysTick_Handler(void)
{
    g_tick_count++;
}

void demo1_board_init(void)
{
    uart_config_t rs485_config;

    BOARD_RdcInit();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    BOARD_InitMemory();

    CLOCK_SetRootMux(kCLOCK_RootUart3, kCLOCK_UartRootmuxSysPll1Div10);
    CLOCK_SetRootDivider(kCLOCK_RootUart3, 1U, 1U);
    CLOCK_EnableClock(kCLOCK_Uart3);

    UART_GetDefaultConfig(&rs485_config);
    rs485_config.baudRate_Bps = DEMO1_RS485_UART_BAUDRATE;
    rs485_config.enableTx = true;
    rs485_config.enableRx = true;
    g_rs485_port.initialized =
        (UART_Init(DEMO1_RS485_UART, &rs485_config, DEMO1_RS485_UART_CLK_FREQ) == kStatus_Success);

    g_tick_count = 0U;
    if (SysTick_Config(SystemCoreClock / (1000000U / DEMO1_TICK_PERIOD_US)) != 0U) {
        for (;;) {
        }
    }
}

void demo1_board_wait_next_tick(void)
{
    uint32_t observed = g_tick_count;

    while (observed == g_tick_count) {
        __WFI();
    }
}

uint64_t demo1_board_now_us(void)
{
    uint32_t count = g_tick_count;

    if (count == 0U) {
        return 0ULL;
    }
    return (uint64_t)(count - 1U) * (uint64_t)DEMO1_TICK_PERIOD_US;
}

void demo1_board_uart_write(const char *buffer, size_t length)
{
    size_t index;

    if (buffer == NULL) {
        return;
    }

    for (index = 0U; index < length; ++index) {
        DbgConsole_Putchar((int)buffer[index]);
    }
}

void *demo1_board_rs485_context(void)
{
    return &g_rs485_port;
}

int demo1_board_rs485_write_bytes(void *context, const uint8_t *data, size_t length)
{
    demo1_board_rs485_port_t *port = (demo1_board_rs485_port_t *)context;

    if (port == NULL || !port->initialized || data == NULL || length == 0U) {
        return -1;
    }
    return UART_WriteBlocking(DEMO1_RS485_UART, data, length) == kStatus_Success ? 0 : -1;
}

int demo1_board_rs485_read_byte(void *context, uint8_t *out_byte)
{
    demo1_board_rs485_port_t *port = (demo1_board_rs485_port_t *)context;

    if (port == NULL || !port->initialized || out_byte == NULL) {
        return -1;
    }
    if (UART_GetStatusFlag(DEMO1_RS485_UART, kUART_RxDataReadyFlag) ||
        UART_GetStatusFlag(DEMO1_RS485_UART, kUART_RxOverrunFlag)) {
        *out_byte = UART_ReadByte(DEMO1_RS485_UART);
        return 1;
    }
    return 0;
}

uint64_t demo1_board_rs485_now_us(void *context)
{
    (void)context;
    return demo1_board_now_us();
}
