/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "adc.h"

#define ADC_ADDR            (*(volatile unsigned long*)(0x7000ad00))
#define ADC_STATUS          (*(volatile unsigned long*)(0x7000ad04))
#define ADC_DATA_1          (*(volatile unsigned long*)(0x7000ad20))
#define ADC_DATA_2          (*(volatile unsigned long*)(0x7000ad24))
#define ADC_INIT            (*(volatile unsigned long*)(0x7000ad2c))

static unsigned short adcdata[NUM_ADC_CHANNELS];

/* Scan ADC so that adcdata[channel] gets updated. */
unsigned short adc_scan(int channel)
{
    unsigned int adc_data_1;
    unsigned int adc_data_2;

    if (channel >= NUM_ADC_CHANNELS)
        return 0;

    /* Start conversion */
    ADC_ADDR |= 0x80000000;

    /* Wait for conversion to complete */
    while((ADC_STATUS & (0x40<<8*channel))==0);

    /* Stop conversion */
    ADC_ADDR &=~ 0x80000000;

    /* ADC_DATA_1 and ADC_DATA_2 are both four bytes, one byte per channel.
       For each channel, ADC_DATA_1 stores the 8-bit msb, ADC_DATA_2 stores the
       2-bit lsb (in bits 0 and 1). Each channel is 10 bits total. */
    adc_data_1 = ((ADC_DATA_1 >> (8*channel)) & 0xff);
    adc_data_2 = ((ADC_DATA_2 >> (8*channel+6)) & 0x3);

    adcdata[channel] = (adc_data_1<<2 | adc_data_2);

#if !(defined(PHILIPS_HDD1630) || defined(PHILIPS_HDD6330))
    /* ADC values read low if PLL is enabled */
    if(PLL_CONTROL & 0x80000000){
        adcdata[channel] += 0x14;
        if(adcdata[channel] > 0x400)
            adcdata[channel] = 0x400;
    }
#endif

    return adcdata[channel];
}

/* Read 10-bit channel data */
unsigned short adc_read(int channel)
{
    return adcdata[channel];
}

static int adc_counter;

static void adc_tick(void)
{
    if(++adc_counter == HZ)
    {
        adc_counter = 0;
        adc_scan(0);
        adc_scan(1);
        adc_scan(2);
        adc_scan(3);
    }
}

/* Figured out from how the OF does things */
void adc_init(void)
{
#if defined(PHILIPS_HDD1630) || defined(PHILIPS_HDD6330)
    ADC_INIT = 0;
#else
    ADC_INIT |= 1;
    ADC_INIT |= 0x40000000;
#endif
    udelay(100);

    /* Reset ADC */
    DEV_RS2 |= 0x20;
    udelay(100);

    DEV_RS2 &=~ 0x20;
    udelay(100);

    /* Enable ADC */
    DEV_EN2 |= 0x20;
    udelay(100);

    ADC_CLOCK_SRC |= 0x3;
    udelay(100);

    ADC_ADDR = 0;
    ADC_ADDR |= 0x40;

#if !(defined(PHILIPS_HDD1630) || defined(PHILIPS_HDD6330))
    ADC_ADDR |= 0x20000000;
    udelay(100);

    ADC_INIT;
    ADC_INIT = 0;
    udelay(100);
#endif

    ADC_STATUS = 0;

    /* Enable channel 0 (battery) */
    DEV_INIT1  &=~0x3;
    ADC_ADDR   |= 0x1000000;
    ADC_STATUS |= 0x20;

    /* Enable channel 1 (unknown) */
    DEV_INIT1  &=~30;
    ADC_ADDR   |= 0x2000000;
    ADC_STATUS |= 0x2000;

#if defined (IRIVER_H10) || defined(IRIVER_H10_5GB) || \
    defined(MROBE_100)
    /* Enable channel 2 (H10:remote) */
    DEV_INIT1  &=~0x300;
    DEV_INIT1  |= 0x100;
    ADC_ADDR   |= 0x4000000;
    ADC_STATUS |= 0x200000;

    /* Enable channel 3 (H10:scroll pad) */
    DEV_INIT1  &=~0x3000;
    DEV_INIT1  |= 0x1000;
    ADC_ADDR   |= 0x8000000;
    ADC_STATUS |= 0x20000000;
#endif

#if defined(PHILIPS_HDD1630) || defined(PHILIPS_HDD6330)
    ADC_INIT |= 0x80000000;
    udelay(100);
    ADC_INIT = 0;
#endif

    /* Force a scan of all channels to get initial values */
    adc_scan(0);
    adc_scan(1);
    adc_scan(2);
    adc_scan(3);

    tick_add_task(adc_tick);
}
