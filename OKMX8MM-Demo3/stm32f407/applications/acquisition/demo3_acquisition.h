#ifndef DEMO3_ACQUISITION_H
#define DEMO3_ACQUISITION_H

#include "../../../common/protocol/demo3_protocol.h"

int demo3_acquisition_init(void);
int demo3_acquisition_read(demo3_sample_t *sample);

#endif
