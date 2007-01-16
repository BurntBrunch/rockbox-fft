#include "kernel.h"
#include "system.h"
#include "panic.h"

#include "lcd.h"
#include <stdio.h>

const int TIMER4_MASK = (1 << 14);
const int LCD_MASK   =  (1 << 16);
const int DMA0_MASK   = (1 << 17);
const int DMA1_MASK   = (1 << 18);
const int DMA2_MASK   = (1 << 19);
const int DMA3_MASK   = (1 << 20);

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

extern void timer4(void);
extern void dma0(void);
extern void dma1(void);
extern void dma3(void);

void irq(void)
{
    int intpending = INTPND;

    SRCPND = intpending; /* Clear this interrupt. */
    INTPND = intpending; /* Clear this interrupt. */

    /* Timer 4 */
    if ((intpending & TIMER4_MASK) != 0)
        timer4();
    else if ((intpending & DMA0_MASK) != 0)
        dma0();
    else
    {
        /* unexpected interrupt */
    }
}

void system_reboot(void)
{
    WTCON = 0;
    WTCNT = WTDAT = 1 ;
    WTCON = 0x21;
    for(;;)
        ;
}

void system_init(void)
{
    /* Turn off un-needed devices */

    /* Turn off all of the UARTS */
    CLKCON &= ~( (1<<10) | (1<<11) |(1<<12) );

    /* Turn off AC97 and Camera */
    CLKCON &= ~( (1<<19) | (1<<20) );

    /* Turn off USB host */
    CLKCON &= ~(1 << 6);

    /* Turn off NAND flash controller */
    CLKCON &= ~(1 << 4);
    
}

void set_cpu_frequency(long frequency)
{
    if (frequency == CPUFREQ_MAX)
    {
        /* FCLK: 300MHz, HCLK: 100MHz, PCLK: 50MHz */
        /* MDIV: 97, PDIV: 1, SDIV: 2 */
        /* HDIV: 3, PDIV: 1 */

        MPLLCON = (97 << 12) | (1 << 4) | 2;
        CLKDIVN = (3 << 1) | 1;
        FREQ = CPUFREQ_MAX;
    }
    else
    {
        /* FCLK: 100MHz, HCLK: 100MHz, PCLK: 50MHz */
        /* MDIV: 62, PDIV: 1, SDIV: 3 */
        /* HDIV: 0, PDIV: 1 */

        MPLLCON = (62 << 12) | (1 << 4) | 3;
        CLKDIVN = (0 << 1) | 1;
        FREQ = CPUFREQ_NORMAL;
    }
}
