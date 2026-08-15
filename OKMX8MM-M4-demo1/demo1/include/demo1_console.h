#ifndef DEMO1_CONSOLE_H
#define DEMO1_CONSOLE_H

#include <stddef.h>

#include "demo1_types.h"

size_t demo1_format_batch_status(const demo1_batch_t *batch,
                                 char *buffer,
                                 size_t capacity);

#endif
