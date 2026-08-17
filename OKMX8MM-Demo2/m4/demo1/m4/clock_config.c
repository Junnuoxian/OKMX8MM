/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "clock_config.h"

#include "fsl_common.h"

void BOARD_InitBootClocks(void)
{
    BOARD_BootClockRUN();
}

const ccm_analog_frac_pll_config_t g_audioPll1Config = {
    .refSel  = kANALOG_PllRefOsc24M,
    .mainDiv = 262U,
    .dsm     = 9437U,
    .preDiv  = 2U,
    .postDiv  = 3U,
};

const ccm_analog_frac_pll_config_t g_audioPll2Config = {
    .refSel  = kANALOG_PllRefOsc24M,
    .mainDiv = 361U,
    .dsm     = 17511U,
    .preDiv  = 3U,
    .postDiv  = 3U,
};

const ccm_analog_integer_pll_config_t g_sysPll1Config = {
    .refSel  = kANALOG_PllRefOsc24M,
    .mainDiv = 400U,
    .preDiv  = 3U,
    .postDiv  = 2U,
};

const ccm_analog_integer_pll_config_t g_sysPll2Config = {
    .refSel  = kANALOG_PllRefOsc24M,
    .mainDiv = 250U,
    .preDiv  = 3U,
    .postDiv  = 1U,
};

const ccm_analog_integer_pll_config_t g_sysPll3Config = {
    .refSel  = kANALOG_PllRefOsc24M,
    .mainDiv = 250U,
    .preDiv  = 2U,
    .postDiv  = 2U,
};

void BOARD_BootClockRUN(void)
{
    CLOCK_SetRootMux(kCLOCK_RootAhb, kCLOCK_AhbRootmuxOsc24M);
    CLOCK_SetRootMux(kCLOCK_RootM4, kCLOCK_M4RootmuxOsc24M);

    CLOCK_InitAudioPll1(&g_audioPll1Config);
    CLOCK_InitAudioPll2(&g_audioPll2Config);

    CLOCK_SetRootDivider(kCLOCK_RootM4, 1U, 2U);
    CLOCK_SetRootMux(kCLOCK_RootM4, kCLOCK_M4RootmuxSysPll1);

    CLOCK_SetRootDivider(kCLOCK_RootAhb, 1U, 1U);
    CLOCK_SetRootMux(kCLOCK_RootAhb, kCLOCK_AhbRootmuxSysPll1Div6);

    CLOCK_SetRootDivider(kCLOCK_RootAudioAhb, 1U, 2U);
    CLOCK_SetRootMux(kCLOCK_RootAudioAhb, kCLOCK_AudioAhbRootmuxSysPll1);

    CLOCK_SetRootMux(kCLOCK_RootUart4, kCLOCK_UartRootmuxSysPll1Div10);
    CLOCK_SetRootDivider(kCLOCK_RootUart4, 1U, 1U);

    CLOCK_EnableClock(kCLOCK_Rdc);
    CLOCK_EnableClock(kCLOCK_Sim_display);
    CLOCK_EnableClock(kCLOCK_Sim_m);
    CLOCK_EnableClock(kCLOCK_Sim_main);
    CLOCK_EnableClock(kCLOCK_Sim_s);
    CLOCK_EnableClock(kCLOCK_Sim_wakeup);
    CLOCK_EnableClock(kCLOCK_Debug);
    CLOCK_EnableClock(kCLOCK_Dram);
    CLOCK_EnableClock(kCLOCK_Sec_Debug);

    SystemCoreClockUpdate();
}
