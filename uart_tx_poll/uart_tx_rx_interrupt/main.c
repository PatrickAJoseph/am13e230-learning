/* SDK includes */
#include "device.h"
#include <driverlib.h>

/**
 *  This example prints a sample message over
 *  UART interface.
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


#define UART_BR_BRD(x)             ((double)((MCLK_FREQUENCY_MHZ/2) * 1000 * 1000)/((double)16 * (double)x))
#define UART_BR_IBRD(x)             (uint32_t)UART_BR_BRD(x)
#define UART_BR_FBRD(x)             (uint32_t)((double)64.0 * ((double)UART_BR_BRD(x)  - (double)(uint32_t)UART_BR_BRD(x)))

void uart_init()
{
    /* Perform initialization of the UNICOMM module based on instructions
     * provided in section 28.3 High-Level Initialization
     */
    UNICOMM4->GPRCM.RSTCTL |= UNICOMM_RSTCTL_RESETASSERT_ASSERT;
    UNICOMM4->GPRCM.PWREN |= (UNICOMM_PWREN_ENABLE_ENABLE | UNICOMM_PWREN_KEY_UNLOCK_W);

    DELAY_CYCLES(10000);

    UNICOMM4->IPMODE |= UNICOMM_IPMODE_SELECT_UART;

    /* Perform initialization as per instructions provided in 29.2.4 Initialization */
    UC4_UART->CTL0 &= ~UNICOMMUART_CTL0_ENABLE_ENABLE;
    UC4_UART->IBRD = UART_BR_IBRD(115200);
    UC4_UART->FBRD = UART_BR_FBRD(115200);
    UC4_UART->CLKSEL = UNICOMMUART_CLKSEL_BUSCLK_SEL_ENABLE;
    UC4_UART->LCRH |= UNICOMMUART_LCRH_WLEN_DATABIT8;
    UC4_UART->CTL0 |= UNICOMMUART_CTL0_HSE_OVS16 | UNICOMMUART_CTL0_TXE_ENABLE | UNICOMMUART_CTL0_RXE_ENABLE;
    UC4_UART->IFLS = UNICOMMUART_IFLS_RXIFLSEL_LVL_NOT_EMPTY;

    UC4_UART->IFLS |= UNICOMMUART_IFLS_RXCLR_ENABLE;
    while(!(UC4_UART->STAT & UNICOMMUART_STAT_RXCLR_SET));

    UC4_UART->IFLS |= UNICOMMUART_IFLS_TXCLR_ENABLE;
    while(!(UC4_UART->STAT & UNICOMMUART_STAT_TXCLR_SET));

    /* Enable UART Rx interrupt. */
    UC4_UART->CPU_INT.IMASK |= UNICOMMUART_CPU_INT_IMASK_RXINT_SET;

    UC4_UART->CPU_INT.ICLR = (uint32_t)0xFFFFFFFFU;

    /* Pin initialization is done here in order to avoid false triggering of 
     * framing error and line break errors.
     */
    /* Set PA0 to UC4 UART TX. */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA0] = IOMUX_PINCM_PC_CONNECTED | (uint32_t)7;

    /* Set PA1 to UC4 UART RX. */
    IOMUX->SECCFG.PINCM[IOMUX_PINCM_PA1] = IOMUX_PINCM_PC_CONNECTED | IOMUX_PA1_UC4_RX_SCL_SCLK | IOMUX_PINCM_INENA_ENABLE;

    DELAY_CYCLES(1000);
    UC4_UART->CTL0 |= UNICOMMUART_CTL0_ENABLE_ENABLE;
    DELAY_CYCLES(1000);
    
    UC4_UART->IFLS |= UNICOMMUART_IFLS_RXCLR_ENABLE;
    while(!(UC4_UART->STAT & UNICOMMUART_STAT_RXCLR_SET));

    /* Enable UART Rx interrupt. */
    NVIC_EnableIRQ(UC4_INT_IRQn);
}

void uart_putc(char c)
{
    UC4_UART->TXDATA = (uint32_t)c;
    while( UC4_UART->STAT & UNICOMMUART_STAT_BUSY_SET );
}

void uart_put(const void* data, size_t size)
{
    const uint8_t* bytes = data;

    while(size)
    {
        while( !( UC4_UART->STAT & UNICOMMUART_STAT_TXFF_SET ) && size)
        {
            UC4_UART->TXDATA = *bytes;
            size--;
            bytes++;
        }

        while(UC4_UART->STAT & UNICOMMUART_STAT_BUSY_SET);
    }
}

#define SYSTICK_NS_TO_TICKS(x)                              \
    ((uint32_t)(((uint64_t)x * (uint64_t)MCLK_FREQUENCY_MHZ) / (uint64_t)1000))

#define SYSTICK_US_TO_TICKS(x)                              \
    ((uint32_t)(((uint64_t)x * (uint64_t)MCLK_FREQUENCY_MHZ)))

volatile uint64_t systick_ticks_ms;

void systick_init()
{
    SysTick->CTRL = (1U << SysTick_CTRL_CLKSOURCE_Pos);
    SysTick->LOAD = SYSTICK_US_TO_TICKS(1000);
    SysTick->VAL = 0;
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    NVIC_EnableIRQ(SysTick_IRQn);
    systick_ticks_ms = 0;
}

void systick_delay_ms(int t)
{
    uint64_t end_ticks = (uint64_t)systick_ticks_ms + (uint64_t)t;

    while(systick_ticks_ms <= end_ticks)
    {
        __asm volatile("wfi");
    }
}

int main()
{
    Device_Init();
 
    flash_init();
    clock_init();
    power_init();
    pinmux_init();
    systick_init();
    uart_init();

    while (1)
    {
        __asm volatile("wfi");
    }

    return 0;
}

void SysTick_Handler(void)
{
    systick_ticks_ms++;
}

volatile int number_of_tx_bytes;
volatile int number_of_rx_bytes;

void UC4_IRQHandler(void)
{
    uint32_t status = UC4_UART->CPU_INT.MIS;
    uint32_t index = UC4_UART->CPU_INT.IIDX;
    uint8_t rx_byte;

    UC4_UART->CPU_INT.ICLR = status;

    if(index & UNICOMMUART_IIDX_STAT_RXIFG)
    {
        rx_byte = UC4_UART->RXDATA;
        UC4_UART->TXDATA = rx_byte;
        number_of_rx_bytes++;
    }
    
    if(index & UNICOMMUART_IIDX_STAT_TXIFG)
    {
        number_of_tx_bytes++;
    }
}