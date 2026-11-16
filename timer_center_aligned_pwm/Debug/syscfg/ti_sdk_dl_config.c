
/* This file is AUTO GENERATED, DO NOT EDIT manually */


/*
 * Copyright (c) 2024, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */

#include "ti_sdk_dl_config.h"

SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_FLASH_init();
    SYSCFG_DL_SYSCTL_CLK_init();
    SYSCFG_DL_initPower();
    SYSCFG_DL_Pinmux_init();
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_APP_TIMER_0_init();
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIO0);
    DL_GPIO_reset(GPIO1);
    DL_GPIO_reset(GPIO2);
    DL_GPIO_reset(GPIO3);

    DL_TimerG_reset(APP_TIMER_0_INST);

    DL_GPIO_enablePower(GPIO0);
    DL_GPIO_enablePower(GPIO1);
    DL_GPIO_enablePower(GPIO2);
    DL_GPIO_enablePower(GPIO3);

    DL_TimerG_enablePower(APP_TIMER_0_INST);
}

SYSCONFIG_WEAK void SYSCFG_DL_Pinmux_init(void)
{
    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM_PC16_X1);
    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM_PC17_X2);

    DL_GPIO_initDigitalOutput(GPIO_GRP_0_PIN_0_IOMUX);

    DL_GPIO_clearPins(GPIO_GRP_0_PORT, GPIO_GRP_0_PIN_0_PIN);
    DL_GPIO_enableOutput(GPIO_GRP_0_PORT, GPIO_GRP_0_PIN_0_PIN);

}

static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {
    .sysPLLRef   = DL_SYSCTL_SYSPLL_REF_HFCLK,
    .inputFreq   = DL_SYSCTL_SYSPLL_INPUT_FREQ_8_16_MHZ,
    .pDiv        = DL_SYSCTL_SYSPLL_PDIV_2,
    .qDiv        = 31,
    .enableCLK1  = DL_SYSCTL_SYSPLL_CLK1_ENABLE,
    .enableCLK0  = DL_SYSCTL_SYSPLL_CLK0_ENABLE,
    .rDivClk1    = DL_SYSCTL_SYSPLL_RDIVCLK1_DIV2,
    .rDivClk0    = DL_SYSCTL_SYSPLL_RDIVCLK0_DIV2,
};
SYSCONFIG_WEAK void  SYSCFG_DL_SYSCTL_CLK_init(void)
{
    DL_SYSCTL_setHFCLKSourceXTAL(255, false);
    DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *)&gSYSPLLConfig);

    // Before switching to PLL output, step down to a lower frequency and gradually increase
    DL_SYSCTL_enablePLLDivider(DL_SYSCTL_PLL_DIVIDER_DIV4);
    DL_SYSCTL_switchMCLKfromSYSOSCtoHSCLK(DL_SYSCTL_HSCLK_SOURCE_SYSPLL);
    DL_SYSCTL_enablePLLDivider(DL_SYSCTL_PLL_DIVIDER_DIV2);
    DL_Common_delayCycles(20);
    while ((DL_SYSCTL_getClockStatus() & SYSCTL_CLKSTATUS_HSCLKMUX_MASK) != DL_SYSCTL_CLK_STATUS_MCLK_SOURCE_HSCLK);
    DL_SYSCTL_disablePLLDivider();
    DL_Common_delayCycles(20);
    while ((DL_SYSCTL_getClockStatus() & SYSCTL_CLKSTATUS_HSCLKMUX_MASK) != DL_SYSCTL_CLK_STATUS_MCLK_SOURCE_HSCLK);


    DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIV_2_DIV_4);
    DL_SYSCTL_setCANCLKSource(DL_SYSCTL_CANCLK_SOURCE_SYSPLL_DIV2);
}

SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{
    DL_SYSCTL_setVREF(DL_SYSCTL_VREF_EXTERNAL_FULL_SCALE);

}


/*
 * Timer clock configuration to be sourced by BUSCLK /  (100000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   1000000 Hz = 100000000 Hz / (DL_TIMER_CLOCK_DIVIDE_1 * (99 + 1))
 */
static const DL_TimerG_ClockConfig gAPP_TIMER_0ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 99U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * APP_TIMER_0_INST_LOAD_VALUE = (1 ms * 1000000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gAPP_TIMER_0TimerConfig = {
    .period     = APP_TIMER_0_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC_UP,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_APP_TIMER_0_init(void) {

    DL_TimerG_setClockConfig(APP_TIMER_0_INST,
        (DL_TimerG_ClockConfig *) &gAPP_TIMER_0ClockConfig);

    DL_TimerG_initTimerMode(APP_TIMER_0_INST,
        (DL_TimerG_TimerConfig *) &gAPP_TIMER_0TimerConfig);

     DL_TimerG_enableInterrupt(APP_TIMER_0_INST, (DL_TIMER_INTERRUPT_ZERO_EVENT));

    DL_TimerG_enableClock(APP_TIMER_0_INST);


}



SYSCONFIG_WEAK void SYSCFG_DL_FLASH_init(void)
{
    // Disable cache before changing wait states
    DL_FRI_disableDLB();
    DL_FRI_disableCache();

    // Set the flash wait states
    DL_FRI_setReadWaitStates(0x3);

    // Enable cache to improve performance of code executed from flash.
    DL_FRI_enableDLB();
    DL_FRI_enableCache();
}