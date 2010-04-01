/*
 * This config file is for the Apple iPod Nano
 */
#define TARGET_TREE /* this target is using the target tree system */

#define IPOD_ARCH 1

#define MODEL_NAME   "Apple iPod Nano 1g"

/* For Rolo and boot loader */
#define MODEL_NUMBER 4

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/* define this if you have recording possibility */
#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_FMRADIO)

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_44)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS  (SAMPR_CAP_44)

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

/* LCD dimensions */
#define LCD_WIDTH  176
#define LCD_HEIGHT 132
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565SWAPPED /* rgb565 byte-swapped */

/* LCD stays visible without backlight - simulator hint */
#define HAVE_TRANSFLECTIVE_LCD

#define CONFIG_KEYPAD IPOD_4G_PAD

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_PCF50605

/* Define if the device can wake from an RTC alarm */
#define HAVE_RTC_ALARM

/* Define this if you can switch on/off the accessory power supply */
#define HAVE_ACCESSORY_SUPPLY

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8975 audio codec */
#define HAVE_WM8975

#define AB_REPEAT_ENABLE 1
#define ACTION_WPSAB_SINGLE ACTION_WPS_BROWSE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
/* We can fade the backlight by using PWM */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_PWM

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      32
#define DEFAULT_BRIGHTNESS_SETTING  16

/* define this if the unit uses a scrollwheel for navigation */
#define HAVE_SCROLLWHEEL
/* define to activate advanced wheel acceleration code */
#define HAVE_WHEEL_ACCELERATION
/* define from which rotation speed [degree/sec] on the acceleration starts */
#define WHEEL_ACCEL_START 270
/* define type of acceleration (1 = ^2, 2 = ^3, 3 = ^4) */
#define WHEEL_ACCELERATION 3

/* Define this if you can detect headphones */
#define HAVE_HEADPHONE_DETECTION

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

#define BATTERY_CAPACITY_DEFAULT 300   /* default battery capacity */
#define BATTERY_CAPACITY_MIN     200   /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX     600   /* max. capacity selectable */
#define BATTERY_CAPACITY_INC      20   /* capacity increment */
#define BATTERY_TYPES_COUNT        1   /* only one type */

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

#define CURRENT_NORMAL     32  /* MP3: ~9h playback out of 300mAh battery */
#define CURRENT_BACKLIGHT  20  /* FIXME: this needs adjusting */
#if defined(HAVE_RECORDING)
#define CURRENT_RECORD     35  /* FIXME: this needs adjusting */
#endif

/* Define Apple remote tuner */
#define CONFIG_TUNER IPOD_REMOTE_TUNER
#define HAVE_RDS_CAP

/* Define this if you have a PortalPlayer PP5022 */
#define CONFIG_CPU PP5022

/* Define this if you want to use the PP5020 i2c interface */
#define CONFIG_I2C I2C_PP5020

/* define this if the hardware can be powered off while charging */
//#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* The size of the flash ROM */
#define FLASH_SIZE 0x100000

/* Define this to the CPU frequency */
#define CPU_FREQ      24000000

#define CONFIG_LCD LCD_IPODNANO

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ARC

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK
#define USE_ROCKBOX_USB
#define USB_VENDOR_ID 0x05ac
#define USB_PRODUCT_ID 0x120a
#define HAVE_USB_HID_MOUSE

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

/* Define this if you can read an absolute wheel position */
#define HAVE_WHEEL_POSITION

#define BOOTFILE_EXT "ipod"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define ICODE_ATTR_TREMOR_NOT_MDCT

#define IPOD_ACCESSORY_PROTOCOL
#define HAVE_SERIAL


/* DMA is used only for reading on PP502x because although reads are ~8x faster
 * writes appear to be ~25% slower.
 */
#define HAVE_ATA_DMA

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY
