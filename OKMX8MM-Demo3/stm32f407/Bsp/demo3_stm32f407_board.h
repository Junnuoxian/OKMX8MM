#ifndef DEMO3_STM32F407_BOARD_H
#define DEMO3_STM32F407_BOARD_H

#include "stm32f4xx_hal.h"

#include "../../../common/protocol/demo3_protocol.h"

#define DEMO3_STM32F407_ANALOG_COUNT DEMO3_ANALOG_CHANNEL_COUNT
#define DEMO3_STM32F407_MODBUS_SLAVE_ID 0x01u
#define DEMO3_STM32F407_MODBUS_START_REGISTER 0u
#define DEMO3_STM32F407_RS485_BAUDRATE 9600u
#define DEMO3_STM32F407_MODBUS_FRAME_TIMEOUT_MS 20u
#define DEMO3_STM32F407_CAN_BASE_ID 0x300u
#define DEMO3_STM32F407_SPEED_PULSES_PER_REV 1u

/* Default map for the standalone F407 board; verify it against the PCB. */
#define DEMO3_STM32F407_DI0_PORT GPIOC
#define DEMO3_STM32F407_DI0_PIN GPIO_PIN_6
#define DEMO3_STM32F407_DI1_PORT GPIOC
#define DEMO3_STM32F407_DI1_PIN GPIO_PIN_7
#define DEMO3_STM32F407_RS485_DE_PORT GPIOB
#define DEMO3_STM32F407_RS485_DE_PIN GPIO_PIN_12

extern const uint32_t demo3_stm32f407_adc_channels[
    DEMO3_STM32F407_ANALOG_COUNT];

int demo3_stm32f407_clock_init(void);
int demo3_stm32f407_hal_init(void);
int demo3_stm32f407_read_digital(uint16_t *digital_bits);
int demo3_stm32f407_read_speed(uint32_t *speed_rpm);
void demo3_stm32f407_rs485_transmit_mode(int enabled);

extern ADC_HandleTypeDef demo3_hadc1;
extern UART_HandleTypeDef demo3_huart3;
extern TIM_HandleTypeDef demo3_htim4;
extern CAN_HandleTypeDef demo3_hcan1;

#endif
