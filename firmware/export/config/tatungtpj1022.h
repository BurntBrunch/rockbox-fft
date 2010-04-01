/*
 * This config file is for the Tatung Elio TPJ-1022
 */

#define TARGET_TREE /* this target is using the target tree system */

#define MODEL_NAME "Tatung Elio TPJ-1022"

/* For Rolo and boot loader */
#define MODEL_NUMBER 15

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/* define this if you have recording possibility */
/*#define HAVE_RECORDING*/ /* TODO: add support for this */

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_48 | \
                         SAMPR_CAP_44 | SAMPR_CAP_32 | SAMPR_CAP_8)

/* define the bitmask of recording sample rates
#define REC_SAMPR_CAPS  (SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_48 | \
                         SAMPR_CAP_44 | SAMPR_CAP_32 | SAMPR_CAP_8) */

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* LCD dimensions */
#define LCD_WIDTH  220
#define LCD_HEIGHT 176
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 

/* #define IRAM_LCDFRAMEBUFFER IDATA_ATTR *//* put the lcd frame buffer in IRAM */

#define CONFIG_KEYPAD TATUNG_TPJ1022_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#ifndef BOOTLOADER
//#define CONFIG_RTC RTC_E8564
#endif

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8731 audio codec */
#define HAVE_WM8731

#define AB_REPEAT_ENABLE 1

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define BATTERY_CAPACITY_DEFAULT 1550 /* default battery capacity
                                        TODO: check this, probably different
                                        for different models too */
#define BATTERY_CAPACITY_MIN 1500  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 1600 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 10   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

/* Hardware controlled charging? FIXME */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* define this if the unit can be powered or charged via USB */
/*#define HAVE_USB_POWER*/

/* Define this if you have a PortalPlayer PP5020 */
#define CONFIG_CPU PP5020

/* Define this if you want to use the PP5020 i2c interface */
#define CONFIG_I2C I2C_PP5020

/* define this if the hardware can be powered off while charging */
/* TODO: should this be set for the H10? */
//#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* Define this to the CPU frequency */
/* TODO: this is probably wrong */
#define CPU_FREQ      11289600

/* Type of LCD */
#define CONFIG_LCD LCD_TPJ1022

#define DEFAULT_CONTRAST_SETTING    19

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ARC

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* enable these for the experimental usb stack ROOLKU */
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x07B4
#define USB_PRODUCT_ID 0x0280

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "elio"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"


/* DMA is used only for reading on PP502x because although reads are ~8x faster
 * writes appear to be ~25% slower.
 */
#define HAVE_ATA_DMA

/* Define this if a programmable hotkey is mapped */
//#define HAVE_HOTKEY
