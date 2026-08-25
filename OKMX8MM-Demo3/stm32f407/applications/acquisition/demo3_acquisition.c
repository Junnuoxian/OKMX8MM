#include "demo3_acquisition.h"

#include <stddef.h>

#include "../../../common/protocol/demo3_protocol.h"
#include "../../Bsp/demo3_stm32f407_board.h"

static uint32_t demo3_sequence;

int demo3_acquisition_init(void)
{
    demo3_sequence = 0u;
    return demo3_stm32f407_hal_init();
}

int demo3_acquisition_read(demo3_sample_t *sample)
{
    uint32_t channel;
    int result = 0;

    if (sample == 0) {
        return -1;
    }
    *sample = (demo3_sample_t){0};
    sample->sequence = demo3_sequence++;
    sample->timestamp_ms = HAL_GetTick();
    sample->flags = DEMO3_SAMPLE_VALID;

    if (HAL_ADC_Start(&demo3_hadc1) != HAL_OK) {
        sample->flags |= DEMO3_SAMPLE_ANALOG_ERROR;
        result = -2;
    } else {
        for (channel = 0u;
               channel < DEMO3_STM32F407_ANALOG_COUNT;
               ++channel) {
            if (HAL_ADC_PollForConversion(&demo3_hadc1, 20u) != HAL_OK) {
                sample->flags |= DEMO3_SAMPLE_ANALOG_ERROR;
                result = -3;
                break;
            }
            sample->analog[channel] =
                (int32_t)HAL_ADC_GetValue(&demo3_hadc1);
        }
        if (HAL_ADC_Stop(&demo3_hadc1) != HAL_OK) {
            sample->flags |= DEMO3_SAMPLE_ANALOG_ERROR;
            result = -4;
        }
    }

    if (demo3_stm32f407_read_digital(&sample->digital_bits) != 0) {
        sample->flags |= DEMO3_SAMPLE_DIGITAL_ERROR;
        result = -5;
    }
    if (demo3_stm32f407_read_speed(&sample->speed_rpm) != 0) {
        sample->flags |= DEMO3_SAMPLE_SPEED_ERROR;
        result = -6;
    }
    return result;
}
