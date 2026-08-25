#include "demo3_acquisition.h"
#include "demo3_can.h"
#include "demo3_modbus_slave.h"
#include "../../Bsp/demo3_stm32f407_board.h"

int main(void)
{
    demo3_sample_t sample;
    uint32_t last_sample_tick;

    HAL_Init();
    if (demo3_stm32f407_clock_init() != 0) {
        return -1;
    }
    if (demo3_acquisition_init() != 0) {
        return -2;
    }
    if (demo3_can_init() != 0) {
        return -3;
    }
    if (demo3_modbus_slave_init() != 0) {
        return -4;
    }

    (void)demo3_acquisition_read(&sample);
    (void)demo3_can_send_sample(&sample);
    last_sample_tick = HAL_GetTick();
    for (;;) {
        if (HAL_GetTick() - last_sample_tick >= 1000u) {
            (void)demo3_acquisition_read(&sample);
            (void)demo3_can_send_sample(&sample);
            last_sample_tick = HAL_GetTick();
        }
        (void)demo3_modbus_slave_poll(&sample);
    }
}
