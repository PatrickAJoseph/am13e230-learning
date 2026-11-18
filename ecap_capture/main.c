/* SDK includes */
#include "device.h"
#include <driverlib.h>

/**
 *  This example generates a 100kHz, 80% duty cycle
 *  PWM signal on PA11
 *  Connect PA11 to PB11 to measured the frequency and
 *  duty cycle or use a logic analyzer to measure them
 */

#define REG_WRITE(reg, mask, pos, value)                 \
    (reg = (reg & ~mask) | (value << pos))

#define TRAP                                            \
{                                                       \
    __asm volatile("bkpt 0");                           \
    while(1);                                           \
};
 
#define TRAP_TIMEOUT(operation, timeout)                \
{                                                       \
    unsigned int count = 0;                             \
                                                        \
    while(count <= timeout)                             \
    {                                                   \
        if(!operation)                                  \
        {                                               \
            break;                                      \
        }                                               \
        count++;                                        \
    }                                                   \
                                                        \
    if(count >= timeout)                                \
    {                                                   \
        TRAP;                                           \
    }                                                   \
}

#define DELAY_CYCLES(x)                                 \
{                                                       \
    unsigned int count = 0;                             \
                                                        \
    while(count < x)                                    \
    {                                                   \
        __asm volatile("nop.w");                        \
        count++;                                        \
    }                                                   \
}

/**
  *  System will be operating at 200MHz clock frequency.
  *  Flash frequency for 0-wait state is 50MHz.
  *  Number of flash wait states is 200MHz/50MHz = 4.
  */
#define MCLK_FREQUENCY_MHZ      (200U)
#define FLASH_FREQUENCY_MHZ     (50U)
#define FLASH_WAIT_STATES       (((MCLK_FREQUENCY_MHZ + FLASH_FREQUENCY_MHZ)/FLASH_FREQUENCY_MHZ) - 2U)

RAMFUNC void flash_set_read_wait_states(uint32_t states)
{
    FRI->FRDCNTL &= ~FRI_FRDCNTL_WS0_MODE_MASK;
    __DSB();
    __ISB();
    FRI->FRDCNTL = (FRI->FRDCNTL & (~FRI_FRDCNTL_RWAIT_MASK)) | ( states << FRI_FRDCNTL_RWAIT_OFS );
}

void flash_init()
{
    /* Disable data line buffer and read cache. */
    FRI->FRD_INTF_CTRL &= ~(FRI_FRD_INTF_CTRL_DLB_EN_MASK | FRI_FRD_INTF_CTRL_CACHE_EN_MASK);

    /**
     *  System will be operating at 200MHz clock frequency.
     *  Flash frequency for 0-wait state is 50MHz.
     *  Number of flash wait states is 200MHz/50MHz - 1 = 3.
     */  
    flash_set_read_wait_states(FLASH_WAIT_STATES);

    /* Enable the data line buffer and read cache. */
    FRI->FRD_INTF_CTRL |= (FRI_FRD_INTF_CTRL_DLB_EN_MASK | FRI_FRD_INTF_CTRL_CACHE_EN_MASK);
}

void clock_init()
{
    /* Clock is initialized following the procedure documented in
     * 3.4.2.4 in the technical reference manual.
     */
    SYSCTL->SOCLOCK.XTALCR |= SYSCTL_XTALCR_OSCOFF_MASK;
    REG_WRITE(SYSCTL->SOCLOCK.HFCLKCLKCFG, SYSCTL_HFCLKCLKCFG_XTALTIME_MASK, SYSCTL_HFCLKCLKCFG_XTALTIME_OFS, 255);
    SYSCTL->SOCLOCK.XTALCR &= ~SYSCTL_XTALCR_OSCOFF_MASK;
    TRAP_TIMEOUT((!(SYSCTL->SOCLOCK.CLKSTATUS & SYSCTL_CLKSTATUS_HFCLKGOOD_MASK)), 0xFFFFFFFU);

    if(SYSCTL->SOCLOCK.CLKSTATUS & SYSCTL_CLKSTATUS_HSCLKSOFF_MASK)
    {
        TRAP;
    }

    /* Configure the system PLL following instructions give in
     * section 3.4.2.3 System Phase-Locked Loop (SYSPLL).
     * Input clock frequency is 25MHz and target output frquency is 200MHz.
     */
    SYSCTL->SOCLOCK.HSCLKEN &= ~SYSCTL_HSCLKEN_SYSPLLEN_MASK;

    TRAP_TIMEOUT((!(SYSCTL->SOCLOCK.CLKSTATUS & SYSCTL_CLKSTATUS_SYSPLLOFF_MASK)), 0xFFFFFFFU);   

    SYSCTL->SOCLOCK.SYSPLLCFG0 |= SYSCTL_SYSPLLCFG0_SYSPLLREF_HFCLK;

    REG_WRITE(SYSCTL->SOCLOCK.SYSPLLCFG1, SYSCTL_SYSPLLCFG1_PDIV_MASK, SYSCTL_SYSPLLCFG1_PDIV_OFS, 1U);
    REG_WRITE(SYSCTL->SOCLOCK.SYSPLLCFG1, SYSCTL_SYSPLLCFG1_QDIV_MASK, SYSCTL_SYSPLLCFG1_QDIV_OFS, 31U);
    REG_WRITE(SYSCTL->SOCLOCK.SYSPLLCFG0, SYSCTL_SYSPLLCFG0_RDIVCLK0_MASK, SYSCTL_SYSPLLCFG0_RDIVCLK0_OFS, 0U);
    REG_WRITE(SYSCTL->SOCLOCK.SYSPLLCFG0, SYSCTL_SYSPLLCFG0_RDIVCLK1_MASK, SYSCTL_SYSPLLCFG0_RDIVCLK1_OFS, 0U); 

    SYSCTL->SOCLOCK.SYSPLLCFG0 |= SYSCTL_SYSPLLCFG0_ENABLECLK0_MASK | SYSCTL_SYSPLLCFG0_ENABLECLK1_MASK;

    SYSCTL->SOCLOCK.SYSPLLPARAM0 = *((volatile uint32_t*)(DL_SYSCTL_SYSPLL_INPUT_FREQ_8_16_MHZ));
    SYSCTL->SOCLOCK.SYSPLLPARAM1 = *((volatile uint32_t*)(DL_SYSCTL_SYSPLL_INPUT_FREQ_8_16_MHZ + (uint32_t)0x4));
    SYSCTL->SOCLOCK.HSCLKEN |= SYSCTL_HSCLKEN_SYSPLLEN_MASK;

    DELAY_CYCLES(1000);
    TRAP_TIMEOUT((!(SYSCTL->SOCLOCK.CLKSTATUS & SYSCTL_CLKSTATUS_SYSPLLGOOD_MASK)), 0xFFFFFFFU);
    DELAY_CYCLES(1000);

    /* Keep MCLK frequency low before enabling MCLK source as HSCLK and
     * then set MCLK source to be directly the HSCLK clock.
     */
    SYSCTL->SOCLOCK.HSCLKCFG &= ~SYSCTL_HSCLKCFG_HSCLKSEL_MASK;
    SYSCTL->SOCLOCK.MCLKCFG |= SYSCTL_MCLKCFG_MCLKDIVCFG_RATIO_1_2_4;

    SYSCTL->SOCLOCK.GENCLKEN |= SYSCTL_GENCLKEN_MCLKEXTDIVEN_ENABLE;
    SYSCTL->SOCLOCK.GENCLKEN |= SYSCTL_GENCLKEN_EXTDIVMCLK_DIV16;

    DELAY_CYCLES(10000);

    SYSCTL->SOCLOCK.MCLKCFG |= SYSCTL_MCLKCFG_USEHSCLK_MASK;
    SYSCTL->SOCLOCK.GENCLKEN &= ~SYSCTL_GENCLKEN_EXTDIVMCLK_MASK;
    SYSCTL->SOCLOCK.GENCLKEN |= SYSCTL_GENCLKEN_EXTDIVMCLK_DIV2;

    DELAY_CYCLES(10000);
    SYSCTL->SOCLOCK.GENCLKEN &= ~SYSCTL_GENCLKEN_MCLKEXTDIVEN_ENABLE;
    DELAY_CYCLES(10000);
}

void power_init()
{
    GPIO0->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);
    GPIO1->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);
    GPIO2->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);
    GPIO3->GPRCM.RSTCTL = (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);

    GPIO0->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
    GPIO1->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
    GPIO2->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
    GPIO3->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
}


void pinmux_init()
{
    /* Crystal oscillator pins are left unconnected since they are analog. */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PC16_X1] = IOMUX_PINCM_PC_UNCONNECTED;
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PC17_X2] = IOMUX_PINCM_PC_UNCONNECTED;

    /* Set PA28 to TIMG12 CCP0 input. */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA28] = IOMUX_PINCM_PC_CONNECTED | IOMUX_PA28_TIMG12_0_CCP0 | IOMUX_PINCM_INENA_ENABLE;

    /* Set PA11 as MCPWM0_1A output. */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA11] = IOMUX_PINCM_PC_CONNECTED | IOMUX_PA11_MCPWM0_1A;

    /* Set PB11 as XBAR input. */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PB11] = IOMUX_PINCM_PC_CONNECTED | IOMUX_PB11_GPIO43 | IOMUX_PINCM_INENA_ENABLE;
}

void pwm_init()
{
    /* Disable TBCLK. */
    SYSCTL->SOCLOCK.PERCLKCR &= ~(1U << SYSCTL_PERCLKCR_TBCLKSYNC_OFS);

    /* Reset peripheral. */
    SYSCTL->SOCLOCK.RSTCTL_CLEAR_MCPERIPH = SYSCTL_RSTCTL_CLEAR_MCPERIPH_RESETSTKYCLR_PWM0_CLEAR | SYSCTL_RSTCTL_CLEAR_MCPERIPH_KEY_UNLOCK;
    SYSCTL->SOCLOCK.RSTCTL_ASSERT_MCPERIPH = SYSCTL_RSTCTL_ASSERT_MCPERIPH_RESETASSERT_PWM0_ASSERT | SYSCTL_RSTCTL_ASSERT_MCPERIPH_KEY_UNLOCK;

    /* Enable power to peripheral. */
    SYSCTL->SOCLOCK.PWREN_MCPERIPH |= SYSCTL_PWREN_MCPERIPH_KEY_UNLOCK | SYSCTL_PWREN_MCPERIPH_PWM0_ENABLE;

    /* Wait for some time for the peripheral to power up. */
    DELAY_CYCLES(1000);

    /* Step 1: Configure the Time Base submodule. */
    /* Divide input source clock by 1. */
    /* MCPWM base clock frequency is 200MHz. */
    MCPWM0->TBCTL = ( 0U << MCPWM_TBCTL_CLKDIV_OFS );

    /* Set counter in up counting mode. */
    MCPWM0->TBCTL |= ( 0U << MCPWM_TBCTL_CTRMODE_OFS );

    /* Set TBPRD to generate 100kHz waveform. */
    /* TBPRD = (f_clk / f_pwm) - 1 = (200e6/100e3) - 1 = 2000 - 1 = 1999. */
    MCPWM0->TBPRD = 1999U;
    MCPWM0->TBPRDS = MCPWM0->TBPRD;

    /* Configure output from MCPWM0_1A to generate a PWM signal of 80% duty cycle. */
    MCPWM0->PWM1_CMPA = (uint32_t)((float)0.8f * (float)MCPWM0->TBPRD);
    MCPWM0->PWM1_CMPAS = MCPWM0->PWM1_CMPA;

    /* Force the PWM output to go high when timer counter reaches zero and low when
     * the counter value matches with the compare vlaue.
     */
    MCPWM0->PWM1_AQCTLA = (2U << MCPWM_PWM1_AQCTLA_ZRO_OFS) | (1U << MCPWM_PWM1_AQCTLA_CAU_OFS);
    MCPWM0->PWM1_AQCTLAS = MCPWM0->PWM1_AQCTLA;

    /* Enable TBCLK. */
    SYSCTL->SOCLOCK.PERCLKCR |= (1U << SYSCTL_PERCLKCR_TBCLKSYNC_OFS);
}

void xbar_init()
{
    /* Reset peripheral. */
    SYSCTL->SOCLOCK.RSTCTL_CLEAR_MCPERIPH = SYSCTL_RSTCTL_CLEAR_MCPERIPH_RESETSTKYCLR_XBAR_CLEAR | SYSCTL_RSTCTL_CLEAR_MCPERIPH_KEY_UNLOCK;
    SYSCTL->SOCLOCK.RSTCTL_ASSERT_MCPERIPH = SYSCTL_RSTCTL_ASSERT_MCPERIPH_RESETASSERT_XBAR_ASSERT | SYSCTL_RSTCTL_ASSERT_MCPERIPH_KEY_UNLOCK;

    /* Enable power to peripheral. */
    SYSCTL->SOCLOCK.PWREN_MCPERIPH |= SYSCTL_PWREN_MCPERIPH_KEY_UNLOCK | SYSCTL_PWREN_MCPERIPH_XBAR_ENABLE;

    /* Wait for some time for the peripheral to power up. */
    DELAY_CYCLES(1000);

    /* Select PB11 as input. */
    INPUTXBAR->INPUTSELECT[0] = 43;

    /* Clear pending events. */
    INPUTXBAR_FLAGS->XBARCLR1 = 0xFFFFFFFFU;
    INPUTXBAR_FLAGS->XBARCLR2 = 0xFFFFFFFFU;    
}

void ecap_init()
{
    /* Reset peripheral. */
    SYSCTL->SOCLOCK.RSTCTL_CLEAR_MCPERIPH = SYSCTL_RSTCTL_CLEAR_MCPERIPH_RESETSTKYCLR_ECAP0_CLEAR | SYSCTL_RSTCTL_CLEAR_MCPERIPH_KEY_UNLOCK;
    SYSCTL->SOCLOCK.RSTCTL_ASSERT_MCPERIPH = SYSCTL_RSTCTL_ASSERT_MCPERIPH_RESETASSERT_ECAP0_ASSERT | SYSCTL_RSTCTL_ASSERT_MCPERIPH_KEY_UNLOCK;

    /* Enable power to peripheral. */
    SYSCTL->SOCLOCK.PWREN_MCPERIPH |= SYSCTL_PWREN_MCPERIPH_KEY_UNLOCK | SYSCTL_PWREN_MCPERIPH_ECAP0_ENABLE;

    /* Wait for some time for the peripheral to power up. */
    DELAY_CYCLES(1000);

    /* Select ECAP0IN as the input. */
    ECAP0->ECCTL0 = (0U << ECAP_ECCTL0_INPUTSEL_OFS);

    /* Enable load of capture registers. */
    ECAP0->ECCTL1 = (ECAP_ECCTL1_CAPLDEN_ECAP_ENABLE);

    /* Configure CAPT1 event to capture the rising edge. */
    ECAP0->ECCTL1 |= (ECAP_ECCTL1_CAP1POL_ECAP_CAP_EVENT1_RISE | ECAP_ECCTL1_CTRRST1_ECAP_DO_NOT_RESET_EVENT1);

    /* Configure CAPT2 event to reset the counter on falling edge. */
    ECAP0->ECCTL1 |= (ECAP_ECCTL1_CAP2POL_ECAP_CAP_EVENT2_FALL | ECAP_ECCTL1_CTRRST2_ECAP_RESET_EVENT2);

    /* Configure ECAP module in capture mode. */
    ECAP0->ECCTL2 = ECAP_ECCTL2_CAP_APWM_ECAP_MODULE | ECAP_ECCTL2_STOP_WRAP_ECAP_STOPEVENT2_WRAPEVENT2 | ECAP_ECCTL2_REARM_ECAP_ARMS_ONESHOT;

    /* Generate interrupt on completion of CAPT2 event. */
    ECAP0->ECEINT = ECAP_ECEINT_CEVT2_ECAP_ENAB_CAP2_INTERRUPT;

    /* Enable interrupt. */
    NVIC_EnableIRQ(ECAP0_INT_IRQn);

    /* Start free-running counter and operate in continuous mode. */
    ECAP0->ECCTL2 |= ECAP_ECCTL2_TSCTRSTOP_ECAP_TSCTR_FREE_RUNNING | ECAP_ECCTL2_CONT_ONESHT_ECAP_OPP_CONT;
}


int main()
{
    Device_Init();

    flash_init();
    clock_init();
    power_init();
    pinmux_init();
    xbar_init();
    ecap_init();
    pwm_init();

    while (1)
    {
        __asm volatile("wfi");
    }

    return 0;
}

float measured_duty;
float measured_frequency;

void TIMG12_0_IRQHandler(void)
{
    uint32_t status = TIMG12->CPU_INT.MIS;
    TIMG12->CPU_INT.ICLR = status;

    measured_duty = (float)( TIMG12->COUNTERREGS.CC_01[1] - TIMG12->COUNTERREGS.CC_01[0] + 1 )/(float)( TIMG12->COUNTERREGS.CC_01[1] + 1);    
    measured_frequency = (float)100000000.0f/(float)( TIMG12->COUNTERREGS.CC_01[1] + 1);
}

double frequency;
double duty;
double t_on_ns;
double t_off_ns;
double t_period_ns;

void ECAP0_IRQHandler(void)
{
    uint32_t status = ECAP0->ECFLG;

    ECAP0->ECCLR = status;

    t_on_ns = 5.0 * (double)(ECAP0->CAP1 + 1); 
    t_period_ns = 5.0 * (double)(ECAP0->CAP2 + 1);
    t_off_ns = t_period_ns - t_on_ns;

    frequency = 1000000000.0 / t_period_ns;
    duty = t_on_ns / t_period_ns;
}
