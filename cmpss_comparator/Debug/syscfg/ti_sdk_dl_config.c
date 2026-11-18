
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
    SYSCFG_DL_APP_CMPSSLITE_0_init();
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIO0);
    DL_GPIO_reset(GPIO1);
    DL_GPIO_reset(GPIO2);
    DL_GPIO_reset(GPIO3);

    DL_SYSCTL_resetPeripheral(DL_SYSCTL_RESET_CMPSS2);

    DL_GPIO_enablePower(GPIO0);
    DL_GPIO_enablePower(GPIO1);
    DL_GPIO_enablePower(GPIO2);
    DL_GPIO_enablePower(GPIO3);

    DL_SYSCTL_enablePower(DL_SYSCTL_PWREN_CMPSS2);
}

SYSCONFIG_WEAK void SYSCFG_DL_Pinmux_init(void)
{
    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM_PC16_X1);
    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM_PC17_X2);

    DL_GPIO_initDigitalOutput(GPIO_GRP_0_PIN_0_IOMUX);

    DL_GPIO_clearPins(GPIO_GRP_0_PORT, GPIO_GRP_0_PIN_0_PIN);
    DL_GPIO_enableOutput(GPIO_GRP_0_PORT, GPIO_GRP_0_PIN_0_PIN);

    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM_PA4);
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


static const DL_CMPSSLITE_Config gAPP_CMPSSLITE_0Config = {
    .enableModule      = true,
    .hysteresis        = DL_CMPSSLITE_HYSTERESIS_NONE,
    .dacSwLoadSelect   = DL_CMPSSLITE_DACVAL_MCLK,
    .highCompSSConfig  =
    {
        .useComp       = true,
        .useDac        = true,
        .useFilter     = true,
        .syncSource    = DL_CMPSSLITE_SYNCSOURCE_MCPWM0_SYNCPER,
        .compConfig =
        {
            .sourceSelect         = DL_CMPSSLITE_INSRC_PIN,
            .invert               = false,
            .tripSourceSelect     = DL_CMPSSLITE_TRIP_ASYNC_COMP,
            .tripOutSourceSelect  = DL_CMPSSLITE_TRIPOUT_ASYNC_COMP,
        },
        .dacConfig =
        {
            .initialDacValue = 0,
        },
        .filterConfig  =
        {
            .initFilter                = false,
            .filtInput                 = DL_CMPSSLITE_FILTIN_COMPOUT,
            .samplePrescale            = 0,
            .sampleWindow              = 1,
            .threshold                 = 1,
            .asyncORLatch              = false,
            .enableLatchResetOnPWMSYNC = false,
        },
    },
    .lowCompSSConfig =
    {
        .useComp     = true,
        .useDac      = true,
        .useFilter   = true,
        .syncSource  = DL_CMPSSLITE_SYNCSOURCE_MCPWM0_SYNCPER,
        .compConfig =
        {
            .sourceSelect        = DL_CMPSSLITE_INSRC_PIN,
            .invert              = false,
            .tripSourceSelect    = DL_CMPSSLITE_TRIP_ASYNC_COMP,
            .tripOutSourceSelect = DL_CMPSSLITE_TRIPOUT_ASYNC_COMP,
        },
        .dacConfig =
        {
            .initialDacValue = 1000,
        },
        .filterConfig =
        {
            .initFilter                = false,
            .filtInput                 = DL_CMPSSLITE_FILTIN_COMPOUT,
            .samplePrescale            = 0,
            .sampleWindow              = 1,
            .threshold                 = 1,
            .asyncORLatch              = false,
            .enableLatchResetOnPWMSYNC = false,
        },
    }
};

SYSCONFIG_WEAK void SYSCFG_DL_APP_CMPSSLITE_0_init(void){
    DL_SYSCTL_enableDACOUT(DL_SYSCTL_DACOUTEN_CMPSS2_DACL);
    DL_CMPSSLITE_init(APP_CMPSSLITE_0_INST, (DL_CMPSSLITE_Config *)&gAPP_CMPSSLITE_0Config);
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