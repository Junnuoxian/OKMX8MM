#ifndef DEMO3_STM32_CAN_H
#define DEMO3_STM32_CAN_H

#include "../../../common/protocol/demo3_protocol.h"

int demo3_can_init(void);
int demo3_can_send_sample(const demo3_sample_t *sample);

#endif
