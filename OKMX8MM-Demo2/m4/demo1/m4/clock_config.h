/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CLOCK_CONFIG_H_
#define _CLOCK_CONFIG_H_

#if defined(__cplusplus)
extern "C" {
#endif

void BOARD_InitBootClocks(void);
void BOARD_BootClockRUN(void);

#if defined(__cplusplus)
}
#endif

#endif /* _CLOCK_CONFIG_H_ */
