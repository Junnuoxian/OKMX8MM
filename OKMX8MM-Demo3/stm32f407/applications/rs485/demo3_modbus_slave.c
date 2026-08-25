#include "demo3_modbus_slave.h"

#include "../../Bsp/demo3_stm32f407_board.h"
#include "demo3_modbus_codec.h"

int demo3_modbus_slave_init(void)
{
    demo3_stm32f407_rs485_transmit_mode(0);
    return 0;
}

int demo3_modbus_slave_poll(const demo3_sample_t *sample)
{
    uint8_t request[8];
    uint8_t response[3u + DEMO3_MODBUS_REGISTER_COUNT * 2u + 2u];
    size_t response_length = 0u;
    HAL_StatusTypeDef status;

    if (sample == 0) {
        return -1;
    }
    status = HAL_UART_Receive(&demo3_huart3, request, 1u,
                              DEMO3_STM32F407_MODBUS_FRAME_TIMEOUT_MS);
    if (status == HAL_TIMEOUT) {
        return 0;
    }
    if (status != HAL_OK) {
        return -2;
    }
    status = HAL_UART_Receive(&demo3_huart3, request + 1u,
                              (uint16_t)(sizeof(request) - 1u),
                              DEMO3_STM32F407_MODBUS_FRAME_TIMEOUT_MS);
    if (status != HAL_OK) {
        return -2;
    }
    if (request[0] != DEMO3_STM32F407_MODBUS_SLAVE_ID) {
        return -3;
    }
    if (demo3_modbus_slave_build_response(request, sizeof(request), sample,
                                          response, sizeof(response),
                                          &response_length) != 0) {
        return -4;
    }
    demo3_stm32f407_rs485_transmit_mode(1);
    status = HAL_UART_Transmit(&demo3_huart3, response,
                               (uint16_t)response_length, 50u);
    demo3_stm32f407_rs485_transmit_mode(0);
    return status == HAL_OK ? 0 : -5;
}
