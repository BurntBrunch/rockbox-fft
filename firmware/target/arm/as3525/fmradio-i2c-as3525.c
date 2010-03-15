/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Bertrik Sikken
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

/*
    This is the fmradio_i2c interface, used by the radio driver
    to communicate with the radio tuner chip.

    It is implemented using the generic i2c driver, which does "bit-banged"
    I2C with a couple of GPIO pins.
 */

#include "as3525.h"
#include "generic_i2c.h"
#include "fmradio_i2c.h"

#if     defined(SANSA_CLIP) || defined(SANSA_C200V2)
#define I2C_GPIO(x) GPIOB_PIN(x)
#define I2C_GPIO_DIR GPIOB_DIR
#define I2C_SCL_PIN 4
#define I2C_SDA_PIN 5

#elif     defined(SANSA_CLIPV2) || defined(SANSA_CLIPPLUS)
#define I2C_GPIO(x) GPIOB_PIN(x)
#define I2C_GPIO_DIR GPIOB_DIR
#define I2C_SCL_PIN 6
#define I2C_SDA_PIN 7

#elif   defined(SANSA_M200V4)
#define I2C_GPIO(x) GPIOD_PIN(x)
#define I2C_GPIO_DIR GPIOD_DIR
#define I2C_SCL_PIN 7
#define I2C_SDA_PIN 6

#elif   defined(SANSA_FUZE) || defined(SANSA_E200V2)
#define I2C_GPIO(x) GPIOA_PIN(x)
#define I2C_GPIO_DIR GPIOA_DIR
#define I2C_SCL_PIN 6
#define I2C_SDA_PIN 7

#else
#error no FM I2C GPIOPIN defines
#endif

static int fm_i2c_bus;

static void fm_scl_hi(void)
{
    I2C_GPIO(I2C_SCL_PIN) = 1 << I2C_SCL_PIN;
}

static void fm_scl_lo(void)
{
    I2C_GPIO(I2C_SCL_PIN) = 0;
}

static void fm_sda_hi(void)
{
    I2C_GPIO(I2C_SDA_PIN) = 1 << I2C_SDA_PIN;
}

static void fm_sda_lo(void)
{
    I2C_GPIO(I2C_SDA_PIN) = 0;
}

static void fm_sda_input(void)
{
    I2C_GPIO_DIR &= ~(1 << I2C_SDA_PIN);
}

static void fm_sda_output(void)
{
    I2C_GPIO_DIR |= 1 << I2C_SDA_PIN;
}

static void fm_scl_input(void)
{
    I2C_GPIO_DIR &= ~(1 << I2C_SCL_PIN);
}

static void fm_scl_output(void)
{
    I2C_GPIO_DIR |= 1 << I2C_SCL_PIN;
}

static int fm_sda(void)
{
    return I2C_GPIO(I2C_SDA_PIN);
}

static int fm_scl(void)
{
    return I2C_GPIO(I2C_SCL_PIN);
}

/* simple and crude delay, used for all delays in the generic i2c driver */
static void fm_delay(void)
{
    volatile int i;

    /* this loop is uncalibrated and could use more sophistication */
    for (i = 0; i < 20; i++) {
    }
}

/* interface towards the generic i2c driver */
static const struct i2c_interface fm_i2c_interface = {
    .scl_hi = fm_scl_hi,
    .scl_lo = fm_scl_lo,
    .sda_hi = fm_sda_hi,
    .sda_lo = fm_sda_lo,
    .sda_input = fm_sda_input,
    .sda_output = fm_sda_output,
    .scl_input = fm_scl_input,
    .scl_output = fm_scl_output,
    .scl = fm_scl,
    .sda = fm_sda,

    .delay_hd_sta = fm_delay,
    .delay_hd_dat = fm_delay,
    .delay_su_dat = fm_delay,
    .delay_su_sto = fm_delay,
    .delay_su_sta = fm_delay,
    .delay_thigh = fm_delay
};

/* initialise i2c for fmradio */
void fmradio_i2c_init(void)
{
    fm_i2c_bus = i2c_add_node(&fm_i2c_interface);
}

int fmradio_i2c_write(unsigned char address, const unsigned char* buf, int count)
{
    return i2c_write_data(fm_i2c_bus, address, -1, buf, count);
}

int fmradio_i2c_read(unsigned char address, unsigned char* buf, int count)
{
    return i2c_read_data(fm_i2c_bus, address, -1, buf, count);
}



