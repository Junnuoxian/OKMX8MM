#include "demo3_acquisition.h"
#include "demo3_can.h"
#include "demo3_modbus_slave.h"

int main(void)
{
    demo3_sample_t sample;

    /* HAL and board clocks will be added after the exact pin plan is confirmed. */
    if (demo3_acquisition_init() != 0) {
        return -1;
    }
    if (demo3_can_init() != 0) {
        return -2;
    }
    if (demo3_modbus_slave_init() != 0) {
        return -3;
    }

    for (;;) {
        if (demo3_acquisition_read(&sample) == 0) {
            (void)demo3_can_send_sample(&sample);
            (void)demo3_modbus_slave_poll(&sample);
        }
    }
}
