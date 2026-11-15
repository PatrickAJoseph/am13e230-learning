/* SDK includes */
#include "device.h"
#include <driverlib.h>

/**
 *  This example toggles the on-board LEDs. 
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
}

uint32_t samples[4] = { (1U<<4), (1U<<27), (1U<<29), (1U<<30) };

void gpio_init()
{
    /* Configure PA4, PA27, PA29 and PA30 as outputs. */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA4] = IOMUX_PINCM_PC_CONNECTED | (uint32_t)1;
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA27] = IOMUX_PINCM_PC_CONNECTED | (uint32_t)1;
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA29] = IOMUX_PINCM_PC_CONNECTED | (uint32_t)1;
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA30] = IOMUX_PINCM_PC_CONNECTED | (uint32_t)1;

    GPIO0->DOE31_0 |= GPIO_DOE31_0_DIO4_ENABLE;
    GPIO0->DOE31_0 |= GPIO_DOE31_0_DIO27_ENABLE;
    GPIO0->DOE31_0 |= GPIO_DOE31_0_DIO29_ENABLE;
    GPIO0->DOE31_0 |= GPIO_DOE31_0_DIO30_ENABLE;

    GPIO0->DMAMASK |= GPIO_DMAMASK_DOUT4_ENABLE;
    GPIO0->DMAMASK |= GPIO_DMAMASK_DOUT27_ENABLE;
    GPIO0->DMAMASK |= GPIO_DMAMASK_DOUT29_ENABLE;
    GPIO0->DMAMASK |= GPIO_DMAMASK_DOUT30_ENABLE;

    /* Configure PB13 as an input. */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PB13] = IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE | (uint32_t)1;
    GPIO1->DOE31_0 &= ~(1U << 13);

    /* Configure DMA trigger to go to DMA controller on event on PB13 button press. */
    GPIO1->GEN_EVENT0.IMASK |= (1U << 13);
    GPIO1->POLARITY15_0 &= ~GPIO_POLARITY15_0_DIO13_MASK;
    GPIO1->POLARITY15_0 |= GPIO_POLARITY15_0_DIO13_RISE;
}

void dma_init()
{
    /* Configure interrupt to occur when DMA channel 0 transfer is done. */
    DMA0->CPU_INT.IMASK |= DMA_IMASK_DMACH0_SET;
    DMA0->CPU_INT.ICLR &= ~DMA0->CPU_INT.MIS;
    NVIC_EnableIRQ(DMA0_INT_IRQn);

    /* Configure DMA transfer to be a hardware generated event from GPIO1 event. */
    /* Refer to table: Table 11-3. DMA Trigger Source Options for trigger sources. */
    DMA0->DMATRIG[0].DMATCTL = DL_DMA_TRIGGER_SOURCE_GPIO1_GEN_EVENT0;
    
    /* Set transfer mode to single. */
    DMA0->DMACHAN[0].DMACTL = DMA_DMACTL_DMATM_SINGLE |  \
                       DMA_DMACTL_DMAEM_NORMAL | \
                       DMA_DMACTL_DMADSTINCR_UNCHANGED | \
                       DMA_DMACTL_DMASRCINCR_INCREMENT | \
                       DMA_DMACTL_DMADSTWDTH_WORD | \
                       DMA_DMACTL_DMASRCWDTH_WORD;
    
    /* Configure source & destination address. */
    DMA0->DMACHAN[0].DMASA = (uint32_t)samples;
    DMA0->DMACHAN[0].DMADA = (uint32_t)&GPIO0->DOUT31_0;
    DMA0->DMACHAN[0].DMASZ = sizeof(samples)/sizeof(samples[0]);

    /* Enable DMA channel. */
    DMA0->DMACHAN[0].DMACTL |= DMA_DMACTL_DMAEN_ENABLE;
}

int main()
{
    Device_Init();
 
    flash_init();
    clock_init();
    power_init();
    pinmux_init();
    gpio_init();
    dma_init();

    while (1)
    {
        __asm volatile("wfi");
    }

    return 0;
}

volatile int dma_transfer_count = 0;

void DMA0_IRQHandler(void)
{
    uint32_t status = DMA0->CPU_INT.MIS;
    uint32_t index = DMA0->CPU_INT.IIDX;

    DMA0->CPU_INT.ICLR = status;

    if( index & DL_DMA_INTERRUPT_IIDX_DMACH0 )
    {
        DMA0->DMACHAN[0].DMASZ = sizeof(samples)/sizeof(samples[0]);
        DMA0->DMACHAN[0].DMACTL |= DMA_DMACTL_DMAEN_ENABLE;

        dma_transfer_count++;       
    }
}