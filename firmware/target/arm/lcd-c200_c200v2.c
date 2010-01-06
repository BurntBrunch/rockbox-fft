/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Mark Arigo
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
#include <sys/types.h>

#include "config.h"
#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "system.h"
#ifdef SANSA_C200V2
#include "dbop-as3525.h"
#endif

/* Display status */
#if MEMORYSIZE > 2
static unsigned lcd_yuv_options SHAREDBSS_ATTR = 0;
#endif
static bool is_lcd_enabled = true;

/* LCD command set for Samsung S6B33B2 */

#define R_NOP                  0x00
#define R_OSCILLATION_MODE     0x02
#define R_DRIVER_OUTPUT_MODE   0x10
#define R_DCDC_SET             0x20
#define R_BIAS_SET             0x22
#define R_DCDC_CLOCK_DIV       0x24
#define R_DCDC_AMP_ONOFF       0x26
#define R_TEMP_COMPENSATION    0x28
#define R_CONTRAST_CONTROL1    0x2a
#define R_CONTRAST_CONTROL2    0x2b
#define R_STANDBY_OFF          0x2c
#define R_STANDBY_ON           0x2d
#define R_DDRAM_BURST_OFF      0x2e
#define R_DDRAM_BURST_ON       0x2f
#define R_ADDRESSING_MODE      0x30
#define R_ROW_VECTOR_MODE      0x32
#define R_N_LINE_INVERSION     0x34
#define R_FRAME_FREQ_CONTROL   0x36
#define R_RED_PALETTE          0x38
#define R_GREEN_PALETTE        0x3a
#define R_BLUE_PALETTE         0x3c
#define R_ENTRY_MODE           0x40
#define R_X_ADDR_AREA          0x42
#define R_Y_ADDR_AREA          0x43
#define R_RAM_SKIP_AREA        0x45
#define R_DISPLAY_OFF          0x50
#define R_DISPLAY_ON           0x51
#define R_SPEC_DISPLAY_PATTERN 0x53
#define R_PARTIAL_DISPLAY_MODE 0x55
#define R_PARTIAL_START_LINE   0x56
#define R_PARTIAL_END_LINE     0x57
#define R_AREA_SCROLL_MODE     0x59
#define R_SCROLL_START_LINE    0x5a
#define R_DATA_FORMAT_SELECT   0x60

#if defined(SANSA_C200)
/* wait for LCD */
static inline void lcd_wait_write(void)
{
    while (LCD1_CONTROL & LCD1_BUSY_MASK);
}

/* send LCD data */
static void lcd_send_pixel(const fb_data data)
{
    lcd_wait_write();
    LCD1_DATA = data >> 8;
    lcd_wait_write();
    LCD1_DATA = data & 0xff;
}

inline void lcd_write_data(const fb_data *data, int width)
{
    do {
        lcd_send_pixel(*data++);
    } while(--width);
}

/* send LCD command */
static void lcd_send_command(unsigned char cmd, unsigned char arg)
{
    lcd_wait_write();
    LCD1_CMD = cmd;
    /* if the argument is 0, we send a NOP (= 0) command */
    lcd_wait_write();
    LCD1_CMD = arg;
}

static inline void c200v1_lcd_init(void)
{
    /* This is from the c200 of bootloader beginning at offset 0xbbf4 */
    outl(inl(0x70000010) & ~0xfc000000, 0x70000010);
    outl(inl(0x70000010), 0x70000010);

    DEV_INIT2 &= ~0x400;
    udelay(10000);

    LCD1_CONTROL &= ~0x4;
    udelay(15);

    LCD1_CONTROL |= 0x4;
    udelay(10);

    LCD1_CONTROL = 0x0084; /* bits (9,10) = 00 -> fastest setting */
    udelay(10000);
}

#define lcd_delay(delay) udelay((delay) * 1000)

#elif defined(SANSA_C200V2)

static inline void lcd_delay(int delay)
{   //TUNEME : delay is in milliseconds
    delay <<= 14;
    while(delay--) ;
}

/* send LCD data */
void lcd_write_data(const fb_data *data, int width)
{
    do {
        DBOP_DOUT = *data << 8 | *data >> 8;
        data++;

        /* Wait if push fifo is full */
        while ((DBOP_STAT & (1<<6)) != 0);
    } while(--width);

    /* While push fifo is not empty */
    while ((DBOP_STAT & (1<<10)) == 0);
}

/* send LCD command */
static void lcd_send_command(unsigned char cmd, unsigned char val)
{
    DBOP_TIMPOL_23 = 0xa167006e;

    DBOP_DOUT = cmd | val << 8;

    while ((DBOP_STAT & (1<<10)) == 0);

    DBOP_TIMPOL_23 = 0xa167e06f;
}

static inline void as3525_dbop_init(void)
{
    CGU_DBOP = (1<<3) | AS3525_DBOP_DIV;

    DBOP_TIMPOL_01 = 0xe167e167;
    DBOP_TIMPOL_23 = 0xe167006e;
    DBOP_CTRL = 0x40008;

    GPIOB_AFSEL = 0xc;
    GPIOC_AFSEL = 0xff;

    DBOP_TIMPOL_23 = 0x6006e;
    DBOP_CTRL = 0x52008;
    DBOP_TIMPOL_01 = 0x6e167;
    DBOP_TIMPOL_23 = 0xa167e06f;

    lcd_delay(20);
}

#endif

/* LCD init */
void lcd_init_device(void)
{
#if defined(SANSA_C200)
    c200v1_lcd_init();
#elif defined(SANSA_C200V2)
    as3525_dbop_init();
#endif

    lcd_send_command(R_STANDBY_OFF, 0);
    lcd_delay(20);

    lcd_send_command(R_OSCILLATION_MODE, 0x01);
    lcd_delay(20);

    lcd_send_command(R_DCDC_AMP_ONOFF, 0x01);
    lcd_delay(20);

    lcd_send_command(R_DCDC_AMP_ONOFF, 0x09);
    lcd_delay(20);

    lcd_send_command(R_DCDC_AMP_ONOFF, 0x0b);
    lcd_delay(20);

    lcd_send_command(R_DCDC_AMP_ONOFF, 0x0f);
    lcd_delay(20);

    lcd_send_command(R_DRIVER_OUTPUT_MODE, 0x07);

    lcd_send_command(R_DCDC_SET, 0x03);

    lcd_send_command(R_DCDC_CLOCK_DIV, 0x03);

    lcd_send_command(R_TEMP_COMPENSATION, 0x01);

    lcd_send_command(R_CONTRAST_CONTROL1, 0x55);

    lcd_send_command(R_ADDRESSING_MODE, 0x10);

    lcd_send_command(R_ROW_VECTOR_MODE, 0x0e);

    lcd_send_command(R_N_LINE_INVERSION, 0x0d);

    lcd_send_command(R_FRAME_FREQ_CONTROL, 0);

    lcd_send_command(R_ENTRY_MODE, 0x82);

    /* vertical dimensions */
    lcd_send_command(R_Y_ADDR_AREA, 0x1a);      /* y1 + 0x1a */
    lcd_send_command(LCD_HEIGHT - 1 + 0x1a, 0); /* y2 + 0x1a */

    /* horizontal dimensions */
    lcd_send_command(R_X_ADDR_AREA, 0); /* x1 */
    lcd_send_command(LCD_WIDTH - 1, 0); /* x2 */

    lcd_delay(100);

    lcd_send_command(R_DISPLAY_ON, 0);
}

/*** hardware configuration ***/
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    lcd_send_command(R_CONTRAST_CONTROL1, val);
}

void lcd_set_invert_display(bool yesno)
{
    /* TODO: Implement lcd_set_invert_display() */
    (void)yesno;
}

#if defined(HAVE_LCD_ENABLE)
void lcd_enable(bool yesno)
{
    if (yesno == is_lcd_enabled)
        return;

    if ((is_lcd_enabled = yesno))
    {
        lcd_send_command(R_STANDBY_OFF, 0);
        lcd_send_command(R_DISPLAY_ON, 0);
        send_event(LCD_EVENT_ACTIVATION, NULL);
    }
    else
    {
        lcd_send_command(R_STANDBY_ON, 0);
    }
}
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return is_lcd_enabled;
}
#endif


/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    lcd_send_command(R_DRIVER_OUTPUT_MODE, yesno ? 0x02 : 0x07);
}

/*** update functions ***/

#if MEMORYSIZE > 2  /* not for C200V2 */
void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(unsigned char const * const src[3],
                                           int width,
                                           int stride,
                                           int x_screen, /* To align dither pattern */
                                           int y_screen);
/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned char const * yuv_src[3];
    off_t z;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    y += 0x1a;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    lcd_send_command(R_ENTRY_MODE, 0x80);

    lcd_send_command(R_X_ADDR_AREA, x);
    lcd_send_command(x + width - 1, 0);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_send_command(R_Y_ADDR_AREA, y);
            lcd_send_command(y + 1, 0);

            lcd_write_yuv420_lines_odither(yuv_src, width, stride, x, y);

            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            y += 2;
        }
         while (--height > 0);
    }
    else
    {
        do
        {
            lcd_send_command(R_Y_ADDR_AREA, y);
            lcd_send_command(y + 1, 0);

            lcd_write_yuv420_lines(yuv_src, width, stride);

            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            y += 2;
        }
         while (--height > 0);
    }
}
#endif /* MEMORYSIZE > 2 */

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    const fb_data *addr;

    if (x + width >= LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height >= LCD_HEIGHT)
        height = LCD_HEIGHT - y;

    if ((width <= 0) || (height <= 0))
        return; /* Nothing left to do. */

    addr = &lcd_framebuffer[y][x];

    if (width <= 1) {
        /* The X end address must be larger than the X start address, so we
         * switch to vertical mode for single column updates and set the
         * window width to 2 */
        lcd_send_command(R_ENTRY_MODE, 0x80);
        lcd_send_command(R_X_ADDR_AREA, x);
        lcd_send_command(x + 1, 0);
    } else {
        lcd_send_command(R_ENTRY_MODE, 0x82);
        lcd_send_command(R_X_ADDR_AREA, x);
        lcd_send_command(x + width - 1, 0);
    }

    lcd_send_command(R_Y_ADDR_AREA, y + 0x1a);
    lcd_send_command(y + height - 1 + 0x1a, 0);

    do {
        lcd_write_data(addr, width);
        addr += LCD_WIDTH;
    } while (--height > 0);
}
