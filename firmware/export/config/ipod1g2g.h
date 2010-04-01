/*
 * This config file is for the Apple iPod 1g and 2g
 */
#define TARGET_TREE /* this target is using the target tree system */

#define IPOD_ARCH 1

#define MODEL_NAME   "Apple iPod 1g/2g"

/* For Rolo and boot loader */
#define MODEL_NUMBER 19

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/* define this if you have recording possibility
#define HAVE_RECORDING */

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_48 | \
                         SAMPR_CAP_44 | SAMPR_CAP_32 | SAMPR_CAP_8)

/* define the bitmask of recording sample rates
#define REC_SAMPR_CAPS  (SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_48 | \
                         SAMPR_CAP_44 | SAMPR_CAP_32 | SAMPR_CAP_8) */

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if the LCD needs to be shutdown */
#define HAVE_LCD_SHUTDOWN

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN
/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#define LCD_DEPTH  2   /* 4 colours - 2bpp */

#define LCD_PIXELFORMAT HORIZONTAL_PACKING

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR       0x000000
#define LCD_BRIGHTCOLOR     0x648764
#define LCD_BL_DARKCOLOR    0x000000
#define LCD_BL_BRIGHTCOLOR  0xdfd8ff

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

#define HAVE_LCD_CONTRAST

/* LCD contrast */
#define MIN_CONTRAST_SETTING        5
#define MAX_CONTRAST_SETTING        63
#define DEFAULT_CONTRAST_SETTING    30 /* Match boot contrast */

#define CONFIG_KEYPAD IPOD_1G2G_PAD

#define HAVE_SCROLLWHEEL

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8731L audio codec */
#define HAVE_WM8721

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* WM8721 has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

/* Define this if the backlight unverts LCD appearance */
#define HAVE_BACKLIGHT_INVERSION

/* Define this if you can detect headphones */
#define HAVE_HEADPHONE_DETECTION

#define BATTERY_CAPACITY_DEFAULT 1200 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1200  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 1900 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

/* Hardware controlled charging? FIXME */
//#define CONFIG_CHARGING CHARGING_SIMPLE

/* FIXME: Don't shutdown on low battery until we have proper suspend. */
#define NO_LOW_BATTERY_SHUTDOWN

/* Define this if you have a PortalPlayer PP5002 */
#define CONFIG_CPU PP5002

/* We're able to shut off power to the HDD */
#define HAVE_ATA_POWER_OFF

/* Define this if you want to use the PP5002 i2c interface */
#define CONFIG_I2C I2C_PP5002

/* define this if the hardware can be powered off while charging */
//#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* The size of the flash ROM */
#define FLASH_SIZE 0x100000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

#define CONFIG_LCD LCD_IPOD2BPP

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_HANDLED_BY_OF
/* actually firewire only, but handled like USB */

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "ipod"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define ICODE_ATTR_TREMOR_NOT_MDCT

#define IRAM_LCDFRAMEBUFFER IBSS_ATTR /* put the lcd frame buffer in IRAM */

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY
