#include "demo3_stm32f407_board.h"

#include <stddef.h>

ADC_HandleTypeDef demo3_hadc1;
UART_HandleTypeDef demo3_huart3;
TIM_HandleTypeDef demo3_htim4;
CAN_HandleTypeDef demo3_hcan1;

const uint32_t demo3_stm32f407_adc_channels[
    DEMO3_STM32F407_ANALOG_COUNT] = {
    ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3,
    ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6, ADC_CHANNEL_7,
    ADC_CHANNEL_8, ADC_CHANNEL_9
};

static int init_gpio(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
               GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = DEMO3_STM32F407_DI0_PIN | DEMO3_STM32F407_DI1_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DEMO3_STM32F407_DI0_PORT, &gpio);

    gpio.Pin = DEMO3_STM32F407_RS485_DE_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEMO3_STM32F407_RS485_DE_PORT, &gpio);
    demo3_stm32f407_rs485_transmit_mode(0);
    return 0;
}

static int init_adc(void)
{
    ADC_ChannelConfTypeDef channel = {0};
    uint32_t i;

    __HAL_RCC_ADC1_CLK_ENABLE();
    demo3_hadc1.Instance = ADC1;
    demo3_hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    demo3_hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    demo3_hadc1.Init.ScanConvMode = ENABLE;
    demo3_hadc1.Init.ContinuousConvMode = DISABLE;
    demo3_hadc1.Init.DiscontinuousConvMode = DISABLE;
    demo3_hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    demo3_hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    demo3_hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    demo3_hadc1.Init.NbrOfConversion = DEMO3_STM32F407_ANALOG_COUNT;
    demo3_hadc1.Init.DMAContinuousRequests = DISABLE;
    demo3_hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&demo3_hadc1) != HAL_OK) {
        return -1;
    }
    channel.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    for (i = 0u; i < DEMO3_STM32F407_ANALOG_COUNT; ++i) {
        channel.Channel = demo3_stm32f407_adc_channels[i];
        channel.Rank = i + 1u;
        if (HAL_ADC_ConfigChannel(&demo3_hadc1, &channel) != HAL_OK) {
            return -2;
        }
    }
    return 0;
}

static int init_uart(void)
{
    __HAL_RCC_USART3_CLK_ENABLE();
    demo3_huart3.Instance = USART3;
    demo3_huart3.Init.BaudRate = DEMO3_STM32F407_RS485_BAUDRATE;
    demo3_huart3.Init.WordLength = UART_WORDLENGTH_8B;
    demo3_huart3.Init.StopBits = UART_STOPBITS_1;
    demo3_huart3.Init.Parity = UART_PARITY_NONE;
    demo3_huart3.Init.Mode = UART_MODE_TX_RX;
    demo3_huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    demo3_huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&demo3_huart3) != HAL_OK) {
        return -1;
    }
    return 0;
}

static int init_speed_timer(void)
{
    TIM_Encoder_InitTypeDef encoder = {0};
    TIM_MasterConfigTypeDef master = {0};

    __HAL_RCC_TIM4_CLK_ENABLE();
    demo3_htim4.Instance = TIM4;
    demo3_htim4.Init.Prescaler = 0u;
    demo3_htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    demo3_htim4.Init.Period = 0xFFFFu;
    demo3_htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    demo3_htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    encoder.EncoderMode = TIM_ENCODERMODE_TI1;
    encoder.IC1Polarity = TIM_ICPOLARITY_RISING;
    encoder.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    encoder.IC1Prescaler = TIM_ICPSC_DIV1;
    encoder.IC1Filter = 3u;
    encoder.IC2Polarity = TIM_ICPOLARITY_RISING;
    encoder.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    encoder.IC2Prescaler = TIM_ICPSC_DIV1;
    encoder.IC2Filter = 3u;
    if (HAL_TIM_Encoder_Init(&demo3_htim4, &encoder) != HAL_OK) {
        return -1;
    }
    master.MasterOutputTrigger = TIM_TRGO_RESET;
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&demo3_htim4, &master) != HAL_OK ||
        HAL_TIM_Encoder_Start(&demo3_htim4, TIM_CHANNEL_ALL) != HAL_OK) {
        return -2;
    }
    return 0;
}

static int init_can(void)
{
    CAN_FilterTypeDef filter = {0};

    __HAL_RCC_CAN1_CLK_ENABLE();
    demo3_hcan1.Instance = CAN1;
    demo3_hcan1.Init.Prescaler = 6u;
    demo3_hcan1.Init.Mode = CAN_MODE_NORMAL;
    demo3_hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    demo3_hcan1.Init.TimeSeg1 = CAN_BS1_11TQ;
    demo3_hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
    demo3_hcan1.Init.TimeTriggeredMode = DISABLE;
    demo3_hcan1.Init.AutoBusOff = ENABLE;
    demo3_hcan1.Init.AutoWakeUp = ENABLE;
    demo3_hcan1.Init.AutoRetransmission = ENABLE;
    demo3_hcan1.Init.ReceiveFifoLocked = DISABLE;
    demo3_hcan1.Init.TransmitFifoPriority = DISABLE;
    if (HAL_CAN_Init(&demo3_hcan1) != HAL_OK) {
        return -1;
    }
    filter.FilterBank = 0u;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0u;
    filter.FilterIdLow = 0u;
    filter.FilterMaskIdHigh = 0u;
    filter.FilterMaskIdLow = 0u;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14u;
    if (HAL_CAN_ConfigFilter(&demo3_hcan1, &filter) != HAL_OK ||
        HAL_CAN_Start(&demo3_hcan1) != HAL_OK) {
        return -2;
    }
    return 0;
}

int demo3_stm32f407_clock_init(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clocks = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscillator.HSEState = RCC_HSE_ON;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PLLM = 4u;
    oscillator.PLL.PLLN = 168u;
    oscillator.PLL.PLLP = RCC_PLLP_DIV2;
    oscillator.PLL.PLLQ = 7u;
    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) {
        return -1;
    }
    clocks.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV4;
    clocks.APB2CLKDivider = RCC_HCLK_DIV2;
    return HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_5) == HAL_OK ? 0 : -2;
}

int demo3_stm32f407_hal_init(void)
{
    if (init_gpio() != 0 || init_adc() != 0 || init_uart() != 0 ||
        init_speed_timer() != 0 || init_can() != 0) {
        return -1;
    }
    return 0;
}

int demo3_stm32f407_read_digital(uint16_t *digital_bits)
{
    if (digital_bits == 0) {
        return -1;
    }
    *digital_bits = 0u;
    if (HAL_GPIO_ReadPin(DEMO3_STM32F407_DI0_PORT,
                         DEMO3_STM32F407_DI0_PIN) == GPIO_PIN_SET) {
        *digital_bits |= 1u << 0u;
    }
    if (HAL_GPIO_ReadPin(DEMO3_STM32F407_DI1_PORT,
                         DEMO3_STM32F407_DI1_PIN) == GPIO_PIN_SET) {
        *digital_bits |= 1u << 1u;
    }
    return 0;
}

int demo3_stm32f407_read_speed(uint32_t *speed_rpm)
{
    static uint32_t previous_tick;
    static uint16_t previous_counter;
    uint32_t now = HAL_GetTick();
    uint16_t counter = (uint16_t)__HAL_TIM_GET_COUNTER(&demo3_htim4);
    uint32_t elapsed = now - previous_tick;
    int16_t delta = (int16_t)(counter - previous_counter);

    if (speed_rpm == 0) {
        return -1;
    }
    if (previous_tick == 0u || elapsed == 0u) {
        *speed_rpm = 0u;
    } else {
        uint32_t pulses = delta < 0 ? (uint32_t)(-delta) : (uint32_t)delta;
        *speed_rpm = (pulses * 60000u) /
                     (elapsed * DEMO3_STM32F407_SPEED_PULSES_PER_REV);
    }
    previous_tick = now;
    previous_counter = counter;
    return 0;
}

void demo3_stm32f407_rs485_transmit_mode(int enabled)
{
    HAL_GPIO_WritePin(DEMO3_STM32F407_RS485_DE_PORT,
                      DEMO3_STM32F407_RS485_DE_PIN,
                      enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* Weak MSP hooks keep the module usable with or without generated CubeMX MSP. */
__weak void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    if (hadc != 0 && hadc->Instance == ADC1) {
        __HAL_RCC_ADC1_CLK_ENABLE();
    }
}

__weak void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio = {0};

    if (huart == 0 || huart->Instance != USART3) {
        return;
    }
    __HAL_RCC_USART3_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &gpio);
}

__weak void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef *timer)
{
    GPIO_InitTypeDef gpio = {0};

    if (timer == 0 || timer->Instance != TIM4) {
        return;
    }
    __HAL_RCC_TIM4_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOB, &gpio);
}

__weak void HAL_CAN_MspInit(CAN_HandleTypeDef *can)
{
    GPIO_InitTypeDef gpio = {0};

    if (can == 0 || can->Instance != CAN1) {
        return;
    }
    __HAL_RCC_CAN1_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &gpio);
}
