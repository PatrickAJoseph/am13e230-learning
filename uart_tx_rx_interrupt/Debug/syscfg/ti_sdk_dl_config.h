
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

#ifndef ti_sdk_dl_config_h
#define ti_sdk_dl_config_h

#include <driverlib.h>
#include <cmsis_compiler.h>

#define SYSCONFIG_WEAK __WEAK


#ifdef __cplusplus
extern "C" {
#endif

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_Pinmux_init(void);
void SYSCFG_DL_FLASH_init(void);


/* Defines for Clock Frequencies */
#define CPUCLK_FREQ_HZ                                                  200000000
#define MCLK_FREQ_HZ                                                    200000000
#define SEMIMCLK_FREQ_HZ                                                100000000
#define QTRMCLK_FREQ_HZ                                                  50000000
#define CANCLK_FREQ_HZ                                                  100000000
#define SYSOSC_FREQ_HZ                                                   32000000
#define LFOSC_FREQ_HZ                                                       32000
#define HFCLK_FREQ_HZ                                                    25000000
#define SYSPLLCLK0_FREQ_HZ                                              200000000
#define SYSPLLCLK1_FREQ_HZ                                              200000000


/*
Macros to call DL_Common_delayCycles to achieve a delay in microseconds / milliseconds.
These macros convert the desired delay in us/ms to the count value expected
by the function.
Note that the function DL_Common_delayCycles only guarantees at least this
number of cycles will be delayed, not that exactly this number of cycles will be
delayed. Please refer to the function documentation for more details.
*/
#define DEVICE_DELAY_US(us)   DL_Common_delayCycles((uint32_t)(us) * (CPUCLK_FREQ_HZ / 1000000))
#define DEVICE_DELAY_MS(ms)   DL_Common_delayCycles((uint32_t)(ms) * (CPUCLK_FREQ_HZ / 1000))




/* Port definition for Pin Group GPIO_GRP_0 */
#define GPIO_GRP_0_PORT                                                  (GPIO0)
#define GPIO_GRP_0_PORT_BASE                                        (GPIO0_BASE)

/* Defines for PIN_0: GPIO0.5 with pinCMx PA5 on package pin 19 */
#define GPIO_GRP_0_PIN_0_PIN                                    (DL_GPIO_PIN(5))
#define GPIO_GRP_0_PIN_0_IOMUX                                 (IOMUX_PINCM_PA5)


void SYSCFG_DL_SYSCTL_CLK_init(void);
void SYSCFG_DL_SYSCTL_init(void);



#ifdef __cplusplus
}
#endif

#endif /* ti_sdk_dl_config_h */