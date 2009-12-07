/*
 * This config file is for the Cowon D2
 */
#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 24

#define MODEL_NAME   "Cowon D2"

#if 0
#define HAVE_USBSTACK
#define USE_ROCKBOX_USB
#define USB_VENDOR_ID 0x0e21
#define USB_PRODUCT_ID 0x0800
#endif


/* Produce a dual-boot bootloader.bin for mktccboot */
#define TCCBOOT

/* define this if you have recording possibility */
//#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_FMRADIO)

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you can flip your LCD */
/* #define HAVE_LCD_FLIP */

/* define this if you can invert the colours on your LCD */
/* #define HAVE_LCD_INVERT */

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

/* FM Tuner */
#define CONFIG_TUNER LV24020LP
#define HAVE_TUNER_PWR_CTRL

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

#define CONFIG_STORAGE (STORAGE_NAND | STORAGE_SD)
#define HAVE_MULTIDRIVE
#define HAVE_HOTSWAP
#define NUM_DRIVES 2

#define CONFIG_NAND NAND_TCC

/* Some (2Gb?) D2s seem to be FAT16 formatted */
#define HAVE_FAT16SUPPORT

/* LCD dimensions */
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
#define LCD_DEPTH  16
#define LCD_PIXELFORMAT 565

/* define this if you have LCD enable function */
#define HAVE_LCD_ENABLE

/* define this to indicate your device's keypad */
#define CONFIG_KEYPAD COWOND2_PAD
#define HAVE_TOUCHSCREEN
#define HAVE_BUTTON_DATA

/* The D2 has either a PCF50606 or PCF50635, RTC_D2 handles both */
#define CONFIG_RTC RTC_D2

/* define this if you have RTC RAM available for settings */
//#define HAVE_RTC_RAM

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Reduce Tremor's ICODE usage */
#define ICODE_ATTR_TREMOR_NOT_MDCT

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE 1

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* The D2 uses a WM8985 codec */
#define HAVE_WM8985

/* Use WM8985 EQ1 & EQ5 as hardware tone controls */
/* #define HAVE_SW_TONE_CONTROLS */

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
/* Enable LCD brightness control */
#define HAVE_BACKLIGHT_BRIGHTNESS
/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_HW_REG

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      14
#define DEFAULT_BRIGHTNESS_SETTING  8

#define CONFIG_I2C I2C_TCC780X

#define BATTERY_CAPACITY_DEFAULT 1600 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1500 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

/* Hardware controlled charging */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* Define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define current usage levels. */
#define CURRENT_NORMAL     88 /* 18 hours from a 1600 mAh battery */  
#define CURRENT_BACKLIGHT  30 /* TBD */ 
#define CURRENT_RECORD     0  /* no recording yet */ 

/* Define this if you have a TCC7801 */
#define CONFIG_CPU TCC7801

/* Define this to the CPU frequency */
#define CPU_FREQ 48000000

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define INCLUDE_TIMEOUT_API

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define CONFIG_LCD LCD_COWOND2

#define BOOTFILE_EXT "d2"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

