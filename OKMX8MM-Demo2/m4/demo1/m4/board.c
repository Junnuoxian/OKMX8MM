/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"

#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_iomuxc.h"
#include "fsl_rdc.h"
#include "pin_mux.h"

void BOARD_InitDebugConsole(void)
{
    uint32_t uartClkSrcFreq = BOARD_DEBUG_UART_CLK_FREQ;

    CLOCK_EnableClock(kCLOCK_Uart4);
    DbgConsole_Init(BOARD_DEBUG_UART_INSTANCE, BOARD_DEBUG_UART_BAUDRATE, BOARD_DEBUG_UART_TYPE, uartClkSrcFreq);
}

void BOARD_InitMemory(void)
{
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
    extern uint32_t Load$$LR$$LR_cache_region$$Base[];
    extern uint32_t Image$$ARM_LIB_STACK$$ZI$$Limit[];
    uint32_t cacheStart = (uint32_t)Load$$LR$$LR_cache_region$$Base;
    uint32_t size       = (cacheStart < 0x20000000U) ? (0U) : ((uint32_t)Image$$ARM_LIB_STACK$$ZI$$Limit - cacheStart);
#else
    extern uint32_t __CACHE_REGION_START[];
    extern uint32_t __CACHE_REGION_SIZE[];
    uint32_t cacheStart = (uint32_t)__CACHE_REGION_START;
    uint32_t size       = (uint32_t)__CACHE_REGION_SIZE;
#endif
    uint32_t i = 0U;

    __DMB();
    MPU->CTRL = 0U;

    MPU->RBAR = (0x20000000U & MPU_RBAR_ADDR_Msk) | MPU_RBAR_VALID_Msk | (0U << MPU_RBAR_REGION_Pos);
    MPU->RASR = (0x1U << MPU_RASR_XN_Pos) | (0x3U << MPU_RASR_AP_Pos) | (0x2U << MPU_RASR_TEX_Pos) |
                (0x3U << MPU_RASR_SRD_Pos) | (28U << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk;

    MPU->RBAR = (0x40000000U & MPU_RBAR_ADDR_Msk) | MPU_RBAR_VALID_Msk | (1U << MPU_RBAR_REGION_Pos);
    MPU->RASR = (0x3U << MPU_RASR_AP_Pos) | (0x1U << MPU_RASR_B_Pos) | (29U << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk;

    MPU->RBAR = (0x80000000U & MPU_RBAR_ADDR_Msk) | MPU_RBAR_VALID_Msk | (2U << MPU_RBAR_REGION_Pos);
    MPU->RASR = (0x3U << MPU_RASR_AP_Pos) | (0x1U << MPU_RASR_B_Pos) | (29U << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk;

    while ((size >> i) > 0x1U) {
        i++;
    }

    if (i != 0U) {
        assert((size & (size - 1U)) == 0U);
        assert(!(cacheStart % size));
        assert(size == (uint32_t)(1U << i));
        assert(i >= 5U);

        MPU->RBAR = (cacheStart & MPU_RBAR_ADDR_Msk) | MPU_RBAR_VALID_Msk | (3U << MPU_RBAR_REGION_Pos);
        MPU->RASR = (0x3U << MPU_RASR_AP_Pos) | (0x1U << MPU_RASR_TEX_Pos) | (0x1U << MPU_RASR_C_Pos) |
                    (0x1U << MPU_RASR_B_Pos) | ((i - 1U) << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk;
    }

    MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;
    __DSB();
    __ISB();

    if ((*(uint32_t *)(CSU_SA_ADDR) & CSU_SA_NSN_M_BIT_MASK) == 0U) {
        *(uint32_t *)(GPV5_BASE_ADDR + FORCE_INCR_OFFSET) =
            *(uint32_t *)(GPV5_BASE_ADDR + FORCE_INCR_OFFSET) | FORCE_INCR_BIT_MASK;
    }
}

void BOARD_RdcInit(void)
{
    rdc_domain_assignment_t assignment = {0};
    uint8_t domainId                   = 0U;

    domainId = RDC_GetCurrentMasterDomainId(RDC);
    if ((0x1U & RDC_GetPeriphAccessPolicy(RDC, kRDC_Periph_RDC, domainId)) != 0U) {
        assignment.domainId = BOARD_DOMAIN_ID;
        RDC_SetMasterDomainAssignment(RDC, kRDC_Master_M4, &assignment);
    }

    CLOCK_EnableClock(kCLOCK_Iomux);
    CLOCK_EnableClock(kCLOCK_Ipmux1);
    CLOCK_EnableClock(kCLOCK_Ipmux2);
    CLOCK_EnableClock(kCLOCK_Ipmux3);
    CLOCK_EnableClock(kCLOCK_Ipmux4);

#if defined(FLASH_TARGET)
    CLOCK_EnableClock(kCLOCK_Qspi);
#endif

    CLOCK_ControlGate(kCLOCK_SysPll1Gate, kCLOCK_ClockNeededAll);
    CLOCK_ControlGate(kCLOCK_SysPll2Gate, kCLOCK_ClockNeededAll);
    CLOCK_ControlGate(kCLOCK_SysPll3Gate, kCLOCK_ClockNeededAll);
    CLOCK_ControlGate(kCLOCK_AudioPll1Gate, kCLOCK_ClockNeededAll);
    CLOCK_ControlGate(kCLOCK_AudioPll2Gate, kCLOCK_ClockNeededAll);
    CLOCK_ControlGate(kCLOCK_VideoPll1Gate, kCLOCK_ClockNeededAll);
}
