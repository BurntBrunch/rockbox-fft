/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Rob Purchase
 * Copyright © 2008 Rafaël Carré
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "kernel.h"
#include "system.h"
#include "panic.h"
#include "ascodec-target.h"
#include "adc.h"
#include "dma-target.h"
#include "clock-target.h"
#include "fmradio_i2c.h"
#include "button-target.h"
#ifndef BOOTLOADER
#include "mmu-arm.h"
#endif
#include "backlight-target.h"

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked));

default_interrupt(INT_WATCHDOG);
default_interrupt(INT_TIMER1);
default_interrupt(INT_TIMER2);
default_interrupt(INT_USB);
default_interrupt(INT_DMAC);
default_interrupt(INT_NAND);
default_interrupt(INT_IDE);
default_interrupt(INT_MCI0);
default_interrupt(INT_MCI1);
default_interrupt(INT_AUDIO);
default_interrupt(INT_SSP);
default_interrupt(INT_I2C_MS);
default_interrupt(INT_I2C_AUDIO);
default_interrupt(INT_I2SIN);
default_interrupt(INT_I2SOUT);
default_interrupt(INT_UART);
default_interrupt(INT_GPIOD);
default_interrupt(RESERVED1); /* Interrupt 17 : unused */
default_interrupt(INT_CGU);
default_interrupt(INT_MEMORY_STICK);
default_interrupt(INT_DBOP);
default_interrupt(RESERVED2); /* Interrupt 21 : unused */
default_interrupt(RESERVED3); /* Interrupt 22 : unused */
default_interrupt(RESERVED4); /* Interrupt 23 : unused */
default_interrupt(RESERVED5); /* Interrupt 24 : unused */
default_interrupt(RESERVED6); /* Interrupt 25 : unused */
default_interrupt(RESERVED7); /* Interrupt 26 : unused */
default_interrupt(RESERVED8); /* Interrupt 27 : unused */
default_interrupt(RESERVED9); /* Interrupt 28 : unused */
default_interrupt(INT_GPIOA);
default_interrupt(INT_GPIOB);
default_interrupt(INT_GPIOC);

static const char * const irqname[] =
{
    "INT_WATCHDOG", "INT_TIMER1", "INT_TIMER2", "INT_USB", "INT_DMAC", "INT_NAND",
    "INT_IDE", "INT_MCI0", "INT_MCI1", "INT_AUDIO", "INT_SSP", "INT_I2C_MS",
    "INT_I2C_AUDIO", "INT_I2SIN", "INT_I2SOUT", "INT_UART", "INT_GPIOD", "RESERVED1",
    "INT_CGU", "INT_MEMORY_STICK", "INT_DBOP", "RESERVED2", "RESERVED3", "RESERVED4",
    "RESERVED5", "RESERVED6", "RESERVED7", "RESERVED8", "RESERVED9", "INT_GPIOA",
    "INT_GPIOB", "INT_GPIOC"
};

static void UIRQ(void)
{
    unsigned int irq_no = 0;
    int status = VIC_IRQ_STATUS;

    if(status == 0)
        panicf("Unhandled IRQ (source unknown!)");

    while((status >>= 1))
        irq_no++;

    panicf("Unhandled IRQ %02X: %s", irq_no, irqname[irq_no]);
}

struct vec_int_src
{
    int source;
    void (*isr) (void);
};

/* Vectored interrupts (16 available) */
struct vec_int_src vec_int_srcs[] =
{
    { INT_SRC_TIMER1, INT_TIMER1 },
    { INT_SRC_TIMER2, INT_TIMER2 },
    { INT_SRC_DMAC, INT_DMAC },
    { INT_SRC_NAND, INT_NAND },
#ifdef HAVE_MULTIDRIVE
    { INT_SRC_MCI0, INT_MCI0 },
#endif
#ifdef HAVE_HOTSWAP
    { INT_SRC_GPIOA, INT_GPIOA, },
#endif
#ifdef HAVE_RECORDING
    { INT_SRC_I2SIN, INT_I2SIN, },
#endif
};

static void setup_vic(void)
{
    volatile unsigned long *vic_vect_addrs = VIC_VECT_ADDRS;
    volatile unsigned long *vic_vect_cntls = VIC_VECT_CNTLS;
    const unsigned int n = sizeof(vec_int_srcs)/sizeof(vec_int_srcs[0]);
    unsigned int i;

    CGU_PERI |= CGU_VIC_CLOCK_ENABLE; /* enable VIC */
    VIC_INT_EN_CLEAR = 0xffffffff; /* disable all interrupt lines */
    VIC_INT_SELECT = 0; /* only IRQ, no FIQ */

    VIC_DEF_VECT_ADDR = (unsigned long)UIRQ;

    for(i = 0; i < n; i++)
    {
        vic_vect_addrs[i] = (unsigned long)vec_int_srcs[i].isr;
        vic_vect_cntls[i] = (1<<5) | vec_int_srcs[i].source;
    }
}

void irq_handler(void)
{
    asm volatile(   "stmfd  sp!, {r0-r5,ip,lr}      \n" /* Store context */
                    "ldr    r5, =0xC6010030         \n" /* VIC_VECT_ADDR */
                    "mov    lr, pc                  \n" /* Return from ISR */
                    "ldr    pc, [r5]                \n" /* execute ISR */
                    "str    r0, [r5]                \n" /* Ack interrupt */
                    "ldmfd  sp!, {r0-r5,ip,lr}      \n" /* Restore context */
                    "subs   pc, lr, #4              \n" /* Return from IRQ */
            );
}

void fiq_handler(void)
{
    asm volatile (
        "subs   pc, lr, #4   \r\n"
    );
}

#if (CONFIG_CPU == AS3525) /* not v2 */
#if defined(BOOTLOADER)
static void sdram_delay(void)
{
    int delay = 1024; /* arbitrary */
    while (delay--) ;
}

/* Use the same initialization than OF */
static void sdram_init(void)
{
    CGU_PERI |= (CGU_EXTMEM_CLOCK_ENABLE|CGU_EXTMEMIF_CLOCK_ENABLE);

    MPMC_CONTROL = 0x1; /* enable MPMC */

    MPMC_DYNAMIC_CONTROL = 0x183; /* SDRAM NOP, all clocks high */
    sdram_delay();

    MPMC_DYNAMIC_CONTROL = 0x103; /* SDRAM PALL, all clocks high */
    sdram_delay();

    MPMC_DYNAMIC_REFRESH = 0x138; /* 0x138 * 16 HCLK ticks between SDRAM refresh cycles */

    MPMC_CONFIG = 0; /* little endian, HCLK:MPMCCLKOUT[3:0] ratio = 1:1 */

    if(MPMC_PERIPH_ID2 & 0xf0)
        MPMC_DYNAMIC_READ_CONFIG = 0x1; /* command delayed, clock out not delayed */

    /* timings */
    MPMC_DYNAMIC_tRP    = 2;
    MPMC_DYNAMIC_tRAS   = 4;
    MPMC_DYNAMIC_tSREX  = 5;
    MPMC_DYNAMIC_tAPR   = 0;
    MPMC_DYNAMIC_tDAL   = 4;
    MPMC_DYNAMIC_tWR    = 2;
    MPMC_DYNAMIC_tRC    = 5;
    MPMC_DYNAMIC_tRFC   = 5;
    MPMC_DYNAMIC_tXSR   = 5;
    MPMC_DYNAMIC_tRRD   = 2;
    MPMC_DYNAMIC_tMRD   = 2;

#if defined(SANSA_CLIP) || defined(SANSA_M200V4) || defined(SANSA_C200V2)
/* 16 bits external bus, low power SDRAM, 16 Mbits = 2 Mbytes */
#define MEMORY_MODEL 0x21

#elif defined(SANSA_E200V2) || defined(SANSA_FUZE) || defined(SANSA_CLIPV2) \
    || defined(SANSA_CLIPPLUS)
/* 16 bits external bus, high performance SDRAM, 64 Mbits = 8 Mbytes */
#define MEMORY_MODEL 0x5

#else
#error "The external memory in your player is unknown"
#endif

    MPMC_DYNAMIC_RASCAS_0 = (2<<8)|2; /* CAS & RAS latency = 2 clock cycles */
    MPMC_DYNAMIC_CONFIG_0 = (MEMORY_MODEL << 7);

    MPMC_DYNAMIC_RASCAS_1 = MPMC_DYNAMIC_CONFIG_1 =
    MPMC_DYNAMIC_RASCAS_2 = MPMC_DYNAMIC_CONFIG_2 =
    MPMC_DYNAMIC_RASCAS_3 = MPMC_DYNAMIC_CONFIG_3 = 0;

    MPMC_DYNAMIC_CONTROL = 0x82; /* SDRAM MODE, MPMCCLKOUT runs continuously */

    /* program the SDRAM mode register */
    /* FIXME: details the exact settings of mode register */
    asm volatile(
        "ldr r4, [%0]\n"
        : : "p"(0x30000000+0x2300*MEM) : "r4");

    MPMC_DYNAMIC_CONTROL = 0x2; /* SDRAM NORMAL, MPMCCLKOUT runs continuously */

    MPMC_DYNAMIC_CONFIG_0 |= (1<<19); /* buffer enable */
}
#else   /* !BOOTLOADER */
void memory_init(void)
{
    ttb_init();
    /* map every region to itself, uncached */
    map_section(0, 0, 4096, CACHE_NONE);

    /* IRAM */
    map_section(0, IRAM_ORIG, 1, CACHE_ALL);
    map_section(0, UNCACHED_ADDR(IRAM_ORIG), 1, CACHE_NONE);

    /* DRAM */
    map_section(0x30000000, DRAM_ORIG, MEMORYSIZE, CACHE_ALL);
    map_section(0x30000000, UNCACHED_ADDR(DRAM_ORIG), MEMORYSIZE, CACHE_NONE);

    /* map 1st mbyte of DRAM at 0x0 to have exception vectors available */
    map_section(0x30000000, 0, 1, CACHE_ALL);

    enable_mmu();
}
#endif /* BOOTLOADER */
#endif /* CONFIG_CPU == AS3525 (not v2) */

void system_init(void)
{
#if CONFIG_CPU == AS3525v2
    /* Init procedure isn't fully understood yet
     * CCU_* registers differ from AS3525
     */
    unsigned int reset_loops = 640;

    CCU_SRC = 0x57D7BF0;
    while(reset_loops--)
        CCU_SRL = CCU_SRL_MAGIC_NUMBER;
    CCU_SRC = CCU_SRL = 0;

    CGU_PERI &= ~0x7f;      /* pclk 24 MHz */
    CGU_PERI |= ((CLK_DIV(AS3525_PLLA_FREQ, AS3525_PCLK_FREQ) - 1) << 2)
                | 1; /* clk_in = PLLA */
#else
    unsigned int reset_loops = 640;

    CCU_SRC = 0x1fffff0
        & ~CCU_SRC_IDE_EN; /* FIXME */
    while(reset_loops--)
        CCU_SRL = CCU_SRL_MAGIC_NUMBER;
    CCU_SRC = CCU_SRL = 0;

    CCU_SCON = 1; /* AHB master's priority configuration :
                     TIC (Test Interface Controller) > DMA > USB > IDE > ARM */

    CGU_PROC = 0;           /* fclk 24 MHz */
    CGU_PERI &= ~0x7f;      /* pclk 24 MHz */

    CGU_PLLASUP = 0;        /* enable PLLA */
    CGU_PLLA = AS3525_PLLA_SETTING;
    while(!(CGU_INTCTRL & (1<<0)));           /* wait until PLLA is locked */
    
#if (AS3525_MCLK_SEL == AS3525_CLK_PLLB)
    CGU_PLLBSUP = 0;        /* enable PLLB */
    CGU_PLLB = AS3525_PLLB_SETTING;
    while(!(CGU_INTCTRL & (1<<1)));           /* wait until PLLB is locked */
#endif

    /*  Set FCLK frequency */
    CGU_PROC = ((AS3525_FCLK_POSTDIV << 4) |
                (AS3525_FCLK_PREDIV  << 2) |
                 AS3525_FCLK_SEL);
    /*  Set PCLK frequency */
    CGU_PERI = ((CGU_PERI & ~0x7F)  |       /* reset divider bits 0:6 */
                 (AS3525_PCLK_DIV0 << 2) |
                 (AS3525_PCLK_DIV1 << 6) |
                  AS3525_PCLK_SEL);

    asm volatile(
        "mrc p15, 0, r0, c1, c0   \n"      /* control register */
        "bic r0, r0, #3<<30       \n"      /* clears bus bits : sets fastbus */
        "mcr p15, 0, r0, c1, c0   \n"
        : : : "r0" );

#ifdef BOOTLOADER
    sdram_init();
#endif  /* BOOTLOADER */

#endif /* CONFIG_CPU == AS3525v2 */

#if 0 /* the GPIO clock is already enabled by the dualboot function */
    CGU_PERI |= CGU_GPIO_CLOCK_ENABLE;
#endif

    /* enable timer interface for TIMER1 & TIMER2 */
    CGU_PERI |= CGU_TIMERIF_CLOCK_ENABLE;

    setup_vic();

    dma_init();

    ascodec_init();

#ifndef BOOTLOADER
    /*  Initialize power management settings */
    ascodec_write(AS3514_CVDD_DCDC3, AS314_CP_DCDC3_SETTING);
#ifdef CONFIG_TUNER
    fmradio_i2c_init();
#endif
#endif /* !BOOTLOADER */
}

void system_reboot(void)
{
    _backlight_off();
    /* use watchdog to reset */
    CGU_PERI |= (CGU_WDOCNT_CLOCK_ENABLE | CGU_WDOIF_CLOCK_ENABLE);
    WDT_LOAD = 1; /* set counter to 1 */
    WDT_CONTROL = 3; /* enable watchdog counter & reset */
    while(1);
}

void system_exception_wait(void)
{
    /* wait until button release (if a button is pressed) */
    while(button_read_device());
    /* then wait until next button press */
    while(!button_read_device());
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#ifndef BOOTLOADER
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    if(frequency == CPUFREQ_MAX)
    {
#ifdef HAVE_ADJUSTABLE_CPU_VOLTAGE
        /* Increasing frequency so boost voltage before change */
        ascodec_write(AS3514_CVDD_DCDC3, (AS314_CP_DCDC3_SETTING | CVDD_1_20));

        /* Some players run a bit low so use 1.175 volts instead of 1.20  */
        /* Wait for voltage to be at least 1.175v before making fclk > 200 MHz */
        while(adc_read(ADC_CVDD) < 470); /* 470 * .0025 = 1.175V */
#endif  /*  HAVE_ADJUSTABLE_CPU_VOLTAGE */

        asm volatile(
            "mrc p15, 0, r0, c1, c0  \n"

#ifdef ASYNCHRONOUS_BUS
            "orr r0, r0, #3<<30      \n"   /* asynchronous bus clocking */
#else
            "bic r0, r0, #3<<30      \n"   /* clear bus bits */
            "orr r0, r0, #1<<30      \n"   /* synchronous bus clocking */
#endif

            "mcr p15, 0, r0, c1, c0  \n"
            : : : "r0" );

        cpu_frequency = CPUFREQ_MAX;
    }
    else
    {
        asm volatile(
            "mrc p15, 0, r0, c1, c0  \n"
            "bic r0, r0, #3<<30      \n"     /* fastbus clocking */
            "mcr p15, 0, r0, c1, c0  \n"
            : : : "r0" );

#ifdef HAVE_ADJUSTABLE_CPU_VOLTAGE
        /* Decreasing frequency so reduce voltage after change */
        ascodec_write(AS3514_CVDD_DCDC3, (AS314_CP_DCDC3_SETTING | CVDD_1_10));
#endif  /*  HAVE_ADJUSTABLE_CPU_VOLTAGE */

        cpu_frequency = CPUFREQ_NORMAL;
    }
}
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */
#endif /* !BOOTLOADER */
