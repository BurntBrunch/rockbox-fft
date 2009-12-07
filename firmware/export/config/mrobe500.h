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

/*
 * This config file is for the M:Robe 500i
 */
#define TARGET_TREE /* this target is using the target tree system */

#define CONFIG_SDRAM_START 0x00900000

#define OLYMPUS_MROBE_500 1
#define MODEL_NAME   "Olympus M:Robe 500i"

/* For Rolo and boot loader */
#define MODEL_NUMBER 22

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/* Define this to add support for ATA DMA */
//#define HAVE_ATA_DMA

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* define this if you want viewport clipping enabled for safe LCD functions */
#define HAVE_VIEWPORT_CLIP

/* LCD dimensions */
#define CONFIG_LCD LCD_MROBE500

/* These defines are used internal to this header */
#define _LCD_RES_QVGA       1
#define _LCD_RES_VGA        2
#define _LCD_PORTRAIT       1
#define _LCD_LANDSCAPE      2

/* Setup the resolution and orientation */
#define _RESOLUTION         _LCD_RES_VGA
#define _ORIENTATION        _LCD_LANDSCAPE

#if _RESOLUTION == _LCD_RES_VGA 
#define LCD_NATIVE_WIDTH 480
#define LCD_NATIVE_HEIGHT 640
#else
#define LCD_NATIVE_WIDTH 240
#define LCD_NATIVE_HEIGHT 320
#endif

/* choose the lcd orientation. CONFIG_ORIENTATION defined in config.h */
#if _ORIENTATION == _LCD_PORTRAIT
/* This is the Portrait setup */
#define LCD_WIDTH  LCD_NATIVE_WIDTH
#define LCD_HEIGHT LCD_NATIVE_HEIGHT
#else
/* Frame buffer stride */
#define LCD_STRIDEFORMAT    VERTICAL_STRIDE

/* This is the Landscape setup */
#define LCD_WIDTH  LCD_NATIVE_HEIGHT
#define LCD_HEIGHT LCD_NATIVE_WIDTH
#endif

#define LCD_DEPTH  16   /* 65k colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

#define MAX_ICON_HEIGHT 35
#define MAX_ICON_WIDTH 35

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
#define HAVE_LCD_SLEEP
#define HAVE_LCD_SLEEP_SETTING

/* remote LCD */
#define HAVE_REMOTE_LCD
#define LCD_REMOTE_WIDTH  79
#define LCD_REMOTE_HEIGHT 16
#define LCD_REMOTE_DEPTH  1

/* Remote display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_REMOTE_DARKCOLOR       0x000000
#define LCD_REMOTE_BRIGHTCOLOR     0x5a915a
#define LCD_REMOTE_BL_DARKCOLOR    0x000000
#define LCD_REMOTE_BL_BRIGHTCOLOR  0x82b4fa

#define LCD_REMOTE_PIXELFORMAT VERTICAL_PACKING

#define CONFIG_REMOTE_KEYPAD MROBE_REMOTE

#define MIN_REMOTE_CONTRAST_SETTING     0
#define MAX_REMOTE_CONTRAST_SETTING     15
#define DEFAULT_REMOTE_CONTRAST_SETTING 7

#define CONFIG_KEYPAD MROBE500_PAD
#define HAVE_TOUCHSCREEN
#define HAVE_BUTTON_DATA

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

//#define HAVE_HARDWARE_BEEP

/* There is no hardware tone control */
#define HAVE_SW_TONE_CONTROLS

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_RX5X348AB

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define HAVE_BACKLIGHT_BRIGHTNESS

#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING          0
#define MAX_BRIGHTNESS_SETTING          127
#define DEFAULT_BRIGHTNESS_SETTING      85 /* OF "full brightness" */
#define DEFAULT_DIMNESS_SETTING         22 /* OF "most dim" */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#if LCD_NATIVE_WIDTH < 480
#define PLUGIN_BUFFER_SIZE 0x100000
#else
/* THe larger screen needs larger bitmaps in plugins */
#define PLUGIN_BUFFER_SIZE 0x200000
#endif

#define HW_SAMPR_CAPS SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11 | SAMPR_CAP_8

#define BATTERY_CAPACITY_DEFAULT 1500 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1000        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2000        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 100         /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

/* define current usage levels */
#define CURRENT_NORMAL     100 /* Measured */
#define CURRENT_BACKLIGHT  100 /* Over 200 mA total measured when on */
#define CURRENT_RECORD     0  /* no recording */

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* Define this if you have a Texas Instruments TSC2100 touch screen */
#define HAVE_TSC2100

/* M66591 register base */
#define M66591_BASE 0x60000000

/* enable these for the usb stack */
#define CONFIG_USBOTG       USBOTG_M66591
#define USE_ROCKBOX_USB

#define HAVE_USBSTACK
//#define HAVE_USB_POWER
//#define USBPOWER_BUTTON     BUTTON_POWER
//#define USBPOWER_BTN_IGNORE BUTTON_TOUCH
/* usb stack and driver settings */
#define USB_NUM_ENDPOINTS   7
#define USB_VENDOR_ID       0x07b4
#define USB_PRODUCT_ID      0x0281
#define HAVE_USB_HID_MOUSE

/* Define this if hardware supports alternate blitting */
#define HAVE_LCD_MODES LCD_MODE_RGB565 | LCD_MODE_YUV | LCD_MODE_PAL256

#define CONFIG_CPU DM320

#define CONFIG_I2C I2C_DM320

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ 87500000

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "mrobe500"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define IRAM_LCDFRAMEBUFFER IBSS_ATTR /* put the lcd frame buffer in IRAM */

