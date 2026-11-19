/* SDK includes */
#include "device.h"
#include <driverlib.h>

/**
 *  This example toggles the on-board LEDs in the interrupt context.
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

    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA4] = IOMUX_PINCM_PC_UNCONNECTED;
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA16] = IOMUX_PINCM_PC_UNCONNECTED;
}

void cmpss_init()
{
    SYSCTL->SOCLOCK.RSTCTL_CLEAR_MCPERIPH = \
    SYSCTL_RSTCTL_CLEAR_MCPERIPH_KEY_UNLOCK | \
    SYSCTL_RSTCTL_CLEAR_MCPERIPH_RESETSTKYCLR_CMPSS2_CLEAR;

    SYSCTL->SOCLOCK.RSTCTL_ASSERT_MCPERIPH = \
    SYSCTL_RSTCTL_ASSERT_MCPERIPH_KEY_UNLOCK | \
    SYSCTL_RSTCTL_ASSERT_MCPERIPH_RESETASSERT_CMPSS2_ASSERT;

    SYSCTL->SOCLOCK.PWREN_MCPERIPH |= \
        SYSCTL_PWREN_MCPERIPH_CMPSS2_ENABLE | \
        SYSCTL_PWREN_MCPERIPH_KEY_UNLOCK;

    DELAY_CYCLES(1000);

    SYSCTL->SOCLOCK.CMPSSCTL = (1U << SYSCTL_CMPSSCTL_CMP2LDACOUTEN_OFS) | \
                               (1U << SYSCTL_CMPSSCTL_CMPSSCTLEN_OFS);

    DELAY_CYCLES(1000);

    CMPSS2->COMPCTL = (1U << CMPSS_LITE_COMPCTL_COMPDACE_OFS);

    CMPSS2->DACLVALS = 1004;
}

void adc_init()
{
    ADC0->GPRCM.RSTCTL = ADC_RSTCTL_KEY_UNLOCK_W |
                         ADC_RSTCTL_RESETASSERT_ASSERT |
                         ADC_RSTCTL_RESETSTKYCLR_CLR;
    
    ADC0->GPRCM.PWREN = ADC_PWREN_ENABLE_ENABLE | ADC_PWREN_KEY_UNLOCK_W;

    DELAY_CYCLES(1000);

    ADC0->ADCCTL1 |= (1U << ADC_ADCCTL1_ADCPWDNZ_OFS);
    ADC0->ADCCTL1 |= (1U << ADC_ADCCTL1_INTPULSEPOS_OFS);

    while(!(ADC0->ADCCTL1 & ADC_ADCCTL1_ADCPWDNZ_MASK))
    {
        
    }

    DELAY_CYCLES(10000000);

    ADC0->ADCCTL2 = 14U << ADC_ADCCTL2_PRESCALE_OFS;

    ADC0->ADCINTSEL = (0U << ADC_ADCINTSEL_INT1SEL_OFS);
    ADC0->ADCINTSEL |= (1U << ADC_ADCINTSEL_INT1E_OFS);

    ADC0->ADCSEQCTL = (15U << ADC_ADCSEQCTL_SEQEND_OFS);

    DL_SYSCTL_setVREF(DL_SYSCTL_VREF_EXTERNAL_FULL_SCALE);
}

uint16_t adc_convert()
{
    ADC0->ADCSOC0CTL = (3U << ADC_ADCSOC0CTL_CHSEL_OFS);

    ADC0->ADCSEQ1CONFIG = (0U << ADC_ADCSEQ1CONFIG_TRIGSEL_OFS);
    ADC0->ADCSEQ1CONFIG |= (255U << ADC_ADCSEQ1CONFIG_ACQPS_OFS);
    ADC0->ADCSEQ1CONFIG |= (1U << ADC_ADCSEQ1CONFIG_SEQENABLE_OFS);
    ADC0->ADCSEQ1CONFIG |= (1U << ADC_ADCSEQ1CONFIG_SEQSWFRC_OFS);

    while((ADC0->ADCCTL1 & ADC_ADCCTL1_ADCBSY_MASK));

    ADC0->ADCINTFLGCLR = ADC_ADCINTFLG_ADCINT1RESULT_MASK | ADC_ADCINTFLG_ADCINT1_MASK;
    ADC0->ADCCTL1 &= ~ADC_ADCCTL1_ADCBSY_MASK;

    return ADC0RESULT->ADCRESULT0;
}

volatile uint16_t adc_result;

extern void SYSCFG_DL_init(void);

int main()
{
    Device_Init();
    SYSCFG_DL_init();

    //flash_init();
    //clock_init();
    //power_init();
    //pinmux_init();
    //cmpss_init();
    //adc_init();
    //extern void SYSCFG_DL_APP_ADC_0_init(void);
    //SYSCFG_DL_APP_ADC_0_init();

    DL_ADC_powerUp(ADC0);
    DELAY_CYCLES(1000000);

    while (1)
    {
        //adc_result = adc_convert();
        DELAY_CYCLES(100000);

        DL_ADC_forceSequencer(ADC0, (DL_ADC_SEQ_NUMBER)0);

        while (DL_ADC_isBusy(ADC0))
        {

        }

        adc_result = DL_ADC_readResult(ADC0RESULT, (DL_ADC_SOC_NUMBER)0);
    }

    return 0;
}