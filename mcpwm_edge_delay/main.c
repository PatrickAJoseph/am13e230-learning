/* SDK includes*/

#include "device.h"
#include <driverlib.h>
/**
 *  This example generates a 100kHz, 80% duty cycle
 *  PWM signal on PA11 with a rising edge delay of 800ns
 *  on PWM-A output. Use logic analyzer to observe the waveforms.
 */
#define REG_WRITE(reg, mask, pos, value)    \
(reg = (reg & ~mask) | (value << pos))

#define TRAP                                                                                       \
    {                                                                                              \
        __asm volatile("bkpt 0");                                                                  \
        while (1)                                                                                  \
            ;                                                                                      \
    };

#define TRAP_TIMEOUT(operation, timeout)                                                           \
    {                                                                                              \
        unsigned int count = 0;                                                                    \
                                                                                                   \
        while (count <= timeout)                                                                   \
        {                                                                                          \
            if (!operation)                                                                        \
            {                                                                                      \
                break;                                                                             \
            }                                                                                      \
                                                                                                   \
            count++;                                                                               \
        }                                                                                          \
                                                                                                   \
        if (count >= timeout)                                                                      \
        {                                                                                          \
            TRAP;                                                                                  \
        }                                                                                          \
    }

#define DELAY_CYCLES(x)                                                                            \
    {                                                                                              \
        unsigned int count = 0;                                                                    \
        while (count < x)                                                                          \
        {                                                                                          \
            __asm volatile("nop.w");                                                               \
            count++;                                                                               \
        }                                                                                          \
    }

/**
  *  System will be operating at 200MHz clock frequency.
  *  Flash frequency for 0-wait state is 50MHz.
  *  Number of flash wait states is 200MHz/50MHz = 4.
  */
#define MCLK_FREQUENCY_MHZ (200U)
#define FLASH_FREQUENCY_MHZ (50U)
#define FLASH_WAIT_STATES (((MCLK_FREQUENCY_MHZ + FLASH_FREQUENCY_MHZ) / FLASH_FREQUENCY_MHZ) - 2U)

RAMFUNC void flash_set_read_wait_states(uint32_t states)
{
    FRI->FRDCNTL &= ~FRI_FRDCNTL_WS0_MODE_MASK;
    __DSB();
    __ISB();
    FRI->FRDCNTL = (FRI->FRDCNTL & (~FRI_FRDCNTL_RWAIT_MASK)) | (states << FRI_FRDCNTL_RWAIT_OFS);
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

    REG_WRITE(SYSCTL->SOCLOCK.HFCLKCLKCFG, SYSCTL_HFCLKCLKCFG_XTALTIME_MASK,
              SYSCTL_HFCLKCLKCFG_XTALTIME_OFS, 255);

    SYSCTL->SOCLOCK.XTALCR &= ~SYSCTL_XTALCR_OSCOFF_MASK;

    TRAP_TIMEOUT((!(SYSCTL->SOCLOCK.CLKSTATUS & SYSCTL_CLKSTATUS_HFCLKGOOD_MASK)), 0xFFFFFFFU);

    if (SYSCTL->SOCLOCK.CLKSTATUS & SYSCTL_CLKSTATUS_HSCLKSOFF_MASK)
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

    REG_WRITE(SYSCTL->SOCLOCK.SYSPLLCFG1, SYSCTL_SYSPLLCFG1_PDIV_MASK, SYSCTL_SYSPLLCFG1_PDIV_OFS,
              1U);

    REG_WRITE(SYSCTL->SOCLOCK.SYSPLLCFG1, SYSCTL_SYSPLLCFG1_QDIV_MASK, SYSCTL_SYSPLLCFG1_QDIV_OFS,
              31U);

    REG_WRITE(SYSCTL->SOCLOCK.SYSPLLCFG0, SYSCTL_SYSPLLCFG0_RDIVCLK0_MASK,
              SYSCTL_SYSPLLCFG0_RDIVCLK0_OFS, 0U);

    REG_WRITE(SYSCTL->SOCLOCK.SYSPLLCFG0, SYSCTL_SYSPLLCFG0_RDIVCLK1_MASK,
              SYSCTL_SYSPLLCFG0_RDIVCLK1_OFS, 0U);

    SYSCTL->SOCLOCK.SYSPLLCFG0 |=
        SYSCTL_SYSPLLCFG0_ENABLECLK0_MASK | SYSCTL_SYSPLLCFG0_ENABLECLK1_MASK;

    SYSCTL->SOCLOCK.SYSPLLPARAM0 = *((volatile uint32_t *)(DL_SYSCTL_SYSPLL_INPUT_FREQ_8_16_MHZ));

    SYSCTL->SOCLOCK.SYSPLLPARAM1 =
        *((volatile uint32_t *)(DL_SYSCTL_SYSPLL_INPUT_FREQ_8_16_MHZ + (uint32_t)0x4));

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
    GPIO0->GPRCM.RSTCTL =
        (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);

    GPIO1->GPRCM.RSTCTL =
        (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);

    GPIO2->GPRCM.RSTCTL =
        (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);

    GPIO3->GPRCM.RSTCTL =
        (GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT);

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

    /* Set PA11 as MCPWM0_1A output. */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA11] = IOMUX_PINCM_PC_CONNECTED | IOMUX_PA11_MCPWM0_1A;

    /* Set PA12 as MCPWM0_1B output. */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA12] = IOMUX_PINCM_PC_CONNECTED | IOMUX_PA12_MCPWM0_1B;
}

void pwm_init()
{
    /* Disable TBCLK. */
    SYSCTL->SOCLOCK.PERCLKCR &= ~(1U << SYSCTL_PERCLKCR_TBCLKSYNC_OFS);

    /* Reset peripheral. */
    SYSCTL->SOCLOCK.RSTCTL_CLEAR_MCPERIPH = SYSCTL_RSTCTL_CLEAR_MCPERIPH_RESETSTKYCLR_PWM0_CLEAR |
                                            SYSCTL_RSTCTL_CLEAR_MCPERIPH_KEY_UNLOCK;

    SYSCTL->SOCLOCK.RSTCTL_ASSERT_MCPERIPH = SYSCTL_RSTCTL_ASSERT_MCPERIPH_RESETASSERT_PWM0_ASSERT |
                                             SYSCTL_RSTCTL_ASSERT_MCPERIPH_KEY_UNLOCK;

    /* Enable power to peripheral. */
    SYSCTL->SOCLOCK.PWREN_MCPERIPH =
        SYSCTL_PWREN_MCPERIPH_KEY_UNLOCK | SYSCTL_PWREN_MCPERIPH_PWM0_ENABLE;

    /* Wait for some time for the peripheral to power up. */
    DELAY_CYCLES(1000);

    /* Step 1: Configure the Time Base submodule. */
    /* Divide input source clock by 1. */
    /* MCPWM base clock frequency is 200MHz. */
    MCPWM0->TBCTL = (0U << MCPWM_TBCTL_CLKDIV_OFS);

    /* Set counter in up counting mode. */
    MCPWM0->TBCTL |= (0U << MCPWM_TBCTL_CTRMODE_OFS);

    /* Set TBPRD to generate 100kHz waveform. */
    /* TBPRD = (f_clk / f_pwm) - 1 = (200e6/100e3) - 1 = 2000 - 1 = 1999. */
    MCPWM0->TBPRD = 1999U;
    MCPWM0->TBPRDS = MCPWM0->TBPRD;

    /* Configure output from MCPWM0_1A to generate a PWM signal of 80% duty cycle. */
    MCPWM0->PWM1_CMPA = (uint32_t)((double)0.8f * (double)MCPWM0->TBPRD);
    MCPWM0->PWM1_CMPAS = MCPWM0->PWM1_CMPA;

    /* Configure output from MCPWM0_1B to generate a PWM signal of 90% duty cycle. */
    MCPWM0->PWM1_CMPB = (uint32_t)((float)0.9f * (float)MCPWM0->TBPRD);
    MCPWM0->PWM1_CMPBS = MCPWM0->PWM1_CMPB;
    /* Force the PWM output to go high when timer counter reaches zero and low when
     * the counter value matches with the compare vlaue.
     */

    MCPWM0->PWM1_AQCTLA = (2U << MCPWM_PWM1_AQCTLA_ZRO_OFS) | (1U << MCPWM_PWM1_AQCTLA_CAU_OFS);
    MCPWM0->PWM1_AQCTLAS = MCPWM0->PWM1_AQCTLA;

    MCPWM0->PWM1_AQCTLB = (2U << MCPWM_PWM1_AQCTLB_ZRO_OFS) | (1U << MCPWM_PWM1_AQCTLB_CBU_OFS);
    MCPWM0->PWM1_AQCTLBS = MCPWM0->PWM1_AQCTLB;

    /* Configure a rising edge delay of 0.8us on A channel.
     * Refer to Figure 26-30. Configuration Options for the Dead-Band Submodule to understand
     * how to configure the switches to generate the rising edge delay.
     */
    /* Set PWMA and PWMB polarity to non-inverting. */
    MCPWM0->DBCTL = (0U << MCPWM_DBCTL_POLSEL_OFS);

    /* Set output to apply rising edge delay on PWMA path. */
    MCPWM0->DBCTL |= (2U << MCPWM_DBCTL_OUT_MODE_OFS);

    /* Set rising edge delay. Each TBCLK frequency is 200MHz which is 5ns */
    /* Required rising edge delay is 800ns/5ns = 160. */
    MCPWM0->DBRED = 160U;
    MCPWM0->DBREDS = MCPWM0->DBRED;

    /* Enable TBCLK. */
    SYSCTL->SOCLOCK.PERCLKCR |= (1U << SYSCTL_PERCLKCR_TBCLKSYNC_OFS);
}

int main()
{
    Device_Init();
    flash_init();
    clock_init();
    power_init();
    pinmux_init();
    pwm_init();

    while (1)
    {
        __asm volatile("wfi");
    }

    return 0;
}