#ifndef DEMO3_MODBUS_SLAVE_H
#define DEMO3_MODBUS_SLAVE_H

#include "../../../common/protocol/demo3_protocol.h"

int demo3_modbus_slave_init(void);
int demo3_modbus_slave_poll(const demo3_sample_t *sample);

#endif
