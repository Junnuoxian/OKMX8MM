#include "demo3_can.h"

#include <stddef.h>

#include "../../Bsp/demo3_stm32f407_board.h"

static void put_u16_le(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)(value >> 8u);
}

static void put_u32_le(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8u) & 0xFFu);
    out[2] = (uint8_t)((value >> 16u) & 0xFFu);
    out[3] = (uint8_t)((value >> 24u) & 0xFFu);
}

int demo3_can_init(void)
{
    return HAL_CAN_GetState(&demo3_hcan1) == HAL_CAN_STATE_LISTENING ? 0 : -1;
}

int demo3_can_send_sample(const demo3_sample_t *sample)
{
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox;
    uint8_t data[8];
    uint32_t frame;

    if (sample == 0) {
        return -1;
    }
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = 8u;
    header.TransmitGlobalTime = DISABLE;
    for (frame = 0u; frame < 6u; ++frame) {
        size_t channel = (size_t)frame * 2u;
        header.StdId = DEMO3_STM32F407_CAN_BASE_ID + frame;
        if (frame < 5u) {
            put_u32_le(data, (uint32_t)sample->analog[channel]);
            put_u32_le(data + 4u,
                       (uint32_t)sample->analog[channel + 1u]);
        } else {
            put_u16_le(data, sample->digital_bits);
            put_u32_le(data + 2u, sample->speed_rpm);
            put_u16_le(data + 6u, sample->flags);
        }
        if (HAL_CAN_AddTxMessage(&demo3_hcan1, &header, data, &mailbox) !=
            HAL_OK) {
            return -2;
        }
    }
    return 0;
}
