/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "cpu.h"
#include "mmu-arm.h"
#include "kernel.h"
#include "system.h"
#include "panic.h"
#include "uart-target.h"
#include "system-arm.h"
#include "spi.h"
#ifdef CREATIVE_ZVx
#include "dma-target.h"
#else
#include "usb-mr500.h"
#endif

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

void irq_handler(void) __attribute__((interrupt ("IRQ"), naked, section(".icode")));
void fiq_handler(void) __attribute__((interrupt ("FIQ"), naked, section(".icode")));

default_interrupt(TIMER0);
default_interrupt(TIMER1);
default_interrupt(TIMER2);
default_interrupt(TIMER3);
default_interrupt(CCD_VD0);
default_interrupt(CCD_VD1);
default_interrupt(CCD_WEN);
default_interrupt(VENC);
default_interrupt(SERIAL0);
default_interrupt(SERIAL1);
default_interrupt(EXT_HOST);
default_interrupt(DSPHINT);
default_interrupt(UART0);
default_interrupt(UART1);
default_interrupt(USB_DMA);
default_interrupt(USB_CORE);
default_interrupt(VLYNQ);
default_interrupt(MTC0);
default_interrupt(MTC1);
default_interrupt(SD_MMC);
default_interrupt(SDIO_MS);
default_interrupt(GIO0);
default_interrupt(GIO1);
default_interrupt(GIO2);
default_interrupt(GIO3);
default_interrupt(GIO4);
default_interrupt(GIO5);
default_interrupt(GIO6);
default_interrupt(GIO7);
default_interrupt(GIO8);
default_interrupt(GIO9);
default_interrupt(GIO10);
default_interrupt(GIO11);
default_interrupt(GIO12);
default_interrupt(GIO13);
default_interrupt(GIO14);
default_interrupt(GIO15);
default_interrupt(PREVIEW0);
default_interrupt(PREVIEW1);
default_interrupt(WATCHDOG);
default_interrupt(I2C);
default_interrupt(CLKC);
default_interrupt(ICE);
default_interrupt(ARMCOM_RX);
default_interrupt(ARMCOM_TX);
default_interrupt(RESERVED);

/* The entry address is equal to base address plus an offset.
 * The offset is based on the priority of the interrupt. So if
 * the priority of an interrupt is changed, the user should also
 * change the offset for the interrupt in the entry table.
 */

static const unsigned short const irqpriority[] = 
{
    IRQ_TIMER0,IRQ_TIMER1,IRQ_TIMER2,IRQ_TIMER3,IRQ_CCD_VD0,IRQ_CCD_VD1,
    IRQ_CCD_WEN,IRQ_VENC,IRQ_SERIAL0,IRQ_SERIAL1,IRQ_EXT_HOST,IRQ_DSPHINT,
    IRQ_UART0,IRQ_UART1,IRQ_USB_DMA,IRQ_USB_CORE,IRQ_VLYNQ,IRQ_MTC0,IRQ_MTC1,
    IRQ_SD_MMC,IRQ_SDIO_MS,IRQ_GIO0,IRQ_GIO1,IRQ_GIO2,IRQ_GIO3,IRQ_GIO4,IRQ_GIO5,
    IRQ_GIO6,IRQ_GIO7,IRQ_GIO8,IRQ_GIO9,IRQ_GIO10,IRQ_GIO11,IRQ_GIO12,IRQ_GIO13,
    IRQ_GIO14,IRQ_GIO15,IRQ_PREVIEW0,IRQ_PREVIEW1,IRQ_WATCHDOG,IRQ_I2C,IRQ_CLKC,
    IRQ_ICE,IRQ_ARMCOM_RX,IRQ_ARMCOM_TX,IRQ_RESERVED
}; /* IRQ priorities, ranging from highest to lowest */

static void (* const irqvector[])(void) __attribute__ ((section(".idata"))) =
{
    TIMER0,TIMER1,TIMER2,TIMER3,CCD_VD0,CCD_VD1,
    CCD_WEN,VENC,SERIAL0,SERIAL1,EXT_HOST,DSPHINT,
    UART0,UART1,USB_DMA,USB_CORE,VLYNQ,MTC0,MTC1,
    SD_MMC,SDIO_MS,GIO0,GIO1,GIO2,GIO3,GIO4,GIO5,
    GIO6,GIO7,GIO8,GIO9,GIO10,GIO11,GIO12,GIO13,
    GIO14,GIO15,PREVIEW0,PREVIEW1,WATCHDOG,I2C,CLKC,
    ICE,ARMCOM_RX,ARMCOM_TX,RESERVED
};

static const char * const irqname[] =
{
    "TIMER0","TIMER1","TIMER2","TIMER3","CCD_VD0","CCD_VD1",
    "CCD_WEN","VENC","SERIAL0","SERIAL1","EXT_HOST","DSPHINT",
    "UART0","UART1","USB_DMA","USB_CORE","VLYNQ","MTC0","MTC1",
    "SD_MMC","SDIO_MS","GIO0","GIO1","GIO2","GIO3","GIO4","GIO5",
    "GIO6","GIO7","GIO8","GIO9","GIO10","GIO11","GIO12","GIO13",
    "GIO14","GIO15","PREVIEW0","PREVIEW1","WATCHDOG","I2C","CLKC",
    "ICE","ARMCOM_RX","ARMCOM_TX","RESERVED"
};

static void UIRQ(void)
{
    unsigned int offset = (IO_INTC_IRQENTRY0>>2)-1;
    panicf("Unhandled IRQ %02X: %s", offset, irqname[offset]);
}

void irq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */

    asm volatile(   "stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                    "sub   sp, sp, #8           \n"); /* Reserve stack */
    unsigned short addr = IO_INTC_IRQENTRY0>>2;
    if(addr != 0)
    {
        addr--;
        irqvector[addr]();
    }
    asm volatile(   "add   sp, sp, #8           \n"   /* Cleanup stack   */
                    "ldmfd sp!, {r0-r7, ip, lr} \n"   /* Restore context */
                    "subs  pc, lr, #4           \n"); /* Return from IRQ */
}

void fiq_handler(void)
{
    /*
     * Based on: linux/arch/arm/kernel/entry-armv.S and system-meg-fx.c
     */

    asm volatile(   "stmfd sp!, {r0-r7, ip, lr} \n"   /* Store context */
                    "sub   sp, sp, #8           \n"); /* Reserve stack */
    unsigned short addr = IO_INTC_FIQENTRY0>>2;
    if(addr != 0)
    {
        addr--;
        irqvector[addr]();
    }
    asm volatile(   "add   sp, sp, #8           \n"   /* Cleanup stack   */
                    "ldmfd sp!, {r0-r7, ip, lr} \n"   /* Restore context */
                    "subs  pc, lr, #4           \n"); /* Return from FIQ */
}

void system_reboot(void)
{
    /* Code taken from linux/include/asm-arm/arch-itdm320-20/system.h at NeuroSVN */
    __asm__ __volatile__(                    
        "mov     ip, #0                                             \n"
        "mcr     p15, 0, ip, c7, c7, 0           @ invalidate cache \n"
        "mcr     p15, 0, ip, c7, c10,4           @ drain WB         \n"
        "mcr     p15, 0, ip, c8, c7, 0           @ flush TLB (v4)   \n"
        "mrc     p15, 0, ip, c1, c0, 0           @ get ctrl register\n"
        "bic     ip, ip, #0x000f                 @ ............wcam \n"
        "bic     ip, ip, #0x2100                 @ ..v....s........ \n"
        "mcr     p15, 0, ip, c1, c0, 0           @ ctrl register    \n"
        "mov     ip, #0xFF000000                                    \n"
        "orr     pc, ip, #0xFF0000               @ ip = 0xFFFF0000  \n"  
        :
        :
        : "cc"
    );
}

void system_exception_wait(void)
{
    /* Mask all Interrupts. */
    IO_INTC_EINT0 = 0;
    IO_INTC_EINT1 = 0;
    IO_INTC_EINT2 = 0;
    while ((IO_GIO_BITSET0&0x01) != 0); /* Wait for power button */
}

void system_init(void)
{
    /* taken from linux/arch/arm/mach-itdm320-20/irq.c */

    /* Clearing all FIQs and IRQs. */
    IO_INTC_IRQ0 = 0xFFFF;
    IO_INTC_IRQ1 = 0xFFFF;
    IO_INTC_IRQ2 = 0xFFFF;

    IO_INTC_FIQ0 = 0xFFFF;
    IO_INTC_FIQ1 = 0xFFFF;
    IO_INTC_FIQ2 = 0xFFFF;

    /* Masking all Interrupts. */
    IO_INTC_EINT0 = 0;
    IO_INTC_EINT1 = 0;
    IO_INTC_EINT2 = 0;

    /* Setting INTC to all IRQs. */
    IO_INTC_FISEL0 = 0;
    IO_INTC_FISEL1 = 0;
    IO_INTC_FISEL2 = 0;
    
    /* setup the clocks */
    IO_CLK_DIV0=0x0003;
    
    /* SDRAM Divide by 3 */
    IO_CLK_DIV1=0x0102;
    IO_CLK_DIV2=0x021F;
    IO_CLK_DIV3=0x1FFF;
    IO_CLK_DIV4=0x1F00;
    
    /* 27 MHz input clock:
     *  PLLA = 27*11/1
     */
    IO_CLK_PLLA=0x80A0;
    IO_CLK_PLLB=0x80C0;
    
    IO_CLK_SEL0=0x017E;
    IO_CLK_SEL1=0x1000;
    IO_CLK_SEL2=0x1001;
    
    /* need to wait before bypassing */
    
    IO_CLK_BYP=0x0000;
    
    /* turn off some unneeded modules */
    IO_CLK_MOD0 &=  ~0x0018;
    IO_CLK_MOD1 =   0x0918;
    IO_CLK_MOD2 =   ~0x7C58;

    /* IRQENTRY only reflects enabled interrupts */
    IO_INTC_RAW = 0;

    IO_INTC_ENTRY_TBA0 = 0;
    IO_INTC_ENTRY_TBA1 = 0;

    int i;
    /* Set interrupt priorities to predefined values */
    for(i = 0; i < 23; i++)
        DM320_REG(0x0540+i*2) = ((irqpriority[i*2+1] & 0x3F) << 8) | (irqpriority[i*2] & 0x3F); /* IO_INTC_PRIORITYx */
    
    /* Turn off all timers */
    IO_TIMER0_TMMD = CONFIG_TIMER0_TMMD_STOP;
    IO_TIMER1_TMMD = CONFIG_TIMER1_TMMD_STOP;
    IO_TIMER2_TMMD = CONFIG_TIMER2_TMMD_STOP;
    IO_TIMER3_TMMD = CONFIG_TIMER3_TMMD_STOP;

#ifndef CREATIVE_ZVx
    /* set GIO26 (reset pin) to output and low */
    IO_GIO_BITCLR1=(1<<10);
    IO_GIO_DIR1&=~(1<<10);
#endif

    uart_init();
    spi_init();
    
#ifdef CREATIVE_ZVx
    dma_init();
#endif

#if !defined(LCD_NATIVE_WIDTH) && !defined(LCD_NATIVE_HEIGHT)
#define LCD_NATIVE_WIDTH    LCD_WIDTH
#define LCD_NATIVE_HEIGHT   LCD_HEIGHT
#endif

#define LCD_FUDGE       LCD_NATIVE_WIDTH%32
#define LCD_BUFFER_SIZE  ((LCD_NATIVE_WIDTH+LCD_FUDGE)*LCD_NATIVE_HEIGHT*2)
#define LCD_TTB_AREA    ((LCD_BUFFER_SIZE>>19)+1)

    /* MMU initialization (Starts data and instruction cache) */
    ttb_init();
    /* Make sure everything is mapped on itself */
    map_section(0, 0, 0x1000, CACHE_NONE);
    
    /* Enable caching for RAM */
    map_section(CONFIG_SDRAM_START, CONFIG_SDRAM_START, MEM, CACHE_ALL);
    /* enable buffered writing for the framebuffer */
    map_section((int)FRAME, (int)FRAME, LCD_TTB_AREA, BUFFERED);
#ifdef CREATIVE_ZVx
    /* mimic OF */
    map_section(0x00100000, 0x00100000,  4, CACHE_NONE);
    map_section(0x04700000, 0x04700000,  2,   BUFFERED);
    map_section(0x40000000, 0x40000000, 16, CACHE_NONE);
    map_section(0x50000000, 0x50000000, 16, CACHE_NONE);
    map_section(0x60000000, 0x60000000, 16, CACHE_NONE);
    map_section(0x80000000, 0x80000000,  1, CACHE_NONE);
#endif
    enable_mmu();
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{
    if (frequency == CPUFREQ_MAX) {
        IO_CLK_DIV0 = 0x0101;  /* 175 MHz ARM */
    } else {
        IO_CLK_DIV0 = 0x0003; /* 87.5 MHz ARM - not much savings, about 3 mA */
    }
}
#endif

/* This function is pretty crude.  It is not acurate to a usec, but errors on
 *  longer.
 */
void udelay(int usec) {
    volatile int temp=usec*(175000/200);
    
    while(temp) {
        temp--;
    }
}

/* This function sets the spefified pin up */
void dm320_set_io (char pin_num, bool input, bool invert, bool irq, bool irqany,
                    bool chat, char func_num )
{
    volatile short  *pio;
    char            reg_offset; /* Holds the offset to the register */
    char            shift_val;  /* Holds the shift offset to the GPIO bit(s) */
    short           io_val;     /* Used as an intermediary to prevent glitchy
                                 *  assignments. */
    
    /* Make sure this is a valid pin number */
    if( (unsigned) pin_num > 40 )
        return;
        
    /* Clamp the function number */
    func_num    &=  0x03;
    
    /* Note that these are integer calculations */
    reg_offset  =   (pin_num / 16);
    shift_val   =   (pin_num - (16 * reg_offset));
    
    /* Handle the direction */
    /* Calculate the pointer to the direction register */
    pio         = &IO_GIO_DIR0 + reg_offset;
    
    if(input)
        *pio        |=  ( 1 << shift_val );
    else
        *pio        &= ~( 1 << shift_val );
    
    /* Handle the inversion */
    /* Calculate the pointer to the inversion register */
    pio         = &IO_GIO_INV0 + reg_offset;
    
    if(invert)
        *pio        |=  ( 1 << shift_val );
    else
        *pio        &= ~( 1 << shift_val );
        
    /* Handle the chat */
    /* Calculate the pointer to the chat register */
    pio         = &IO_GIO_CHAT0 + reg_offset;
    
    if(chat)
        *pio        |=  ( 1 << shift_val );
    else
        *pio        &= ~( 1 << shift_val );
        
    /* Handle interrupt configuration */
    if(pin_num < 16)
    {
        /* Sets whether the pin is an irq or not */
        if(irq)
            IO_GIO_IRQPORT |=  (1 << pin_num );
        else
            IO_GIO_IRQPORT &= ~(1 << pin_num );
        
        /* Set whether this is a falling or any edge sensitive irq */
        if(irqany)
            IO_GIO_IRQEDGE |=  (1 << pin_num );
        else
            IO_GIO_IRQEDGE &= ~(1 << pin_num );
    }
    
    /* Handle the function number */
    /* Calculate the pointer to the function register */
    reg_offset  =   (  (pin_num - 9) / 8 );
    shift_val   =   ( ((pin_num - 9) - (8 * reg_offset)) * 2 );
    
    if( pin_num < 9 )
    {
        reg_offset  =   0;
        shift_val   =   0;
    }

    /* Calculate the pointer to the function register */
    pio         =   &IO_GIO_FSEL0 + reg_offset;

    io_val      =   *pio;
    io_val      &= ~( 3 << shift_val );         /* zero previous value */
    io_val      |=  ( func_num << shift_val );  /* Store new value */
    *pio        =   io_val;
}

