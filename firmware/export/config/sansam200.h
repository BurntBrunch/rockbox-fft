/*
 * This config file is for the Sansa M200 series
 */
#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 29

#define MODEL_NAME   "Sandisk Sansa m200 series"

/* Enable FAT16 support */
#define HAVE_FAT16SUPPORT

/* define this if you have recording possibility */
//#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
//#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_SPDIF)

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN
/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

#define CONFIG_STORAGE STORAGE_NAND

#define CONFIG_NAND NAND_TCC

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 64
#define LCD_DEPTH  1

#define LCD_PIXELFORMAT VERTICAL_PACKING

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_BARKCOLOR       0x000000
#define LCD_BRIGHTCOLOR     0x5e6854
#define LCD_BL_DARKCOLOR    0x000000
#define LCD_BL_BRIGHTCOLOR  0x3ca0e6

/* define this to indicate your device's keypad */
#define CONFIG_KEYPAD SANSA_M200_PAD

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_TCC77X

/* define this if you have RTC RAM available for settings */
//#define HAVE_RTC_RAM

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x38000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x10000

#define AB_REPEAT_ENABLE 1

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have the TLV320 audio codec */
#define HAVE_TLV320

/* TLV320 has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define CONFIG_I2C I2C_TCC77X

#define BATTERY_CAPACITY_DEFAULT 1500 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1500 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

/* define this if the unit should not shut down on low battery. */
#define NO_LOW_BATTERY_SHUTDOWN

/* Define this if you have a TCC770 */
#define CONFIG_CPU TCC770

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Define this to the CPU frequency */
#define CPU_FREQ      120000000

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define CONFIG_LCD LCD_SSD1815

#define BOOTFILE_EXT "m200"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/"

#define IBSS_ATTR_VOICE_STACK
#define ICODE_ATTR_TREMOR_NOT_MDCT
#define ICODE_ATTR_TREMOR_MDCT
#define ICODE_ATTR_FLAC
#define IBSS_ATTR_FLAC_DECODED0
#define ICONST_ATTR_MPA_HUFFMAN
#define IBSS_ATTR_MPC_SAMPLE_BUF
#define ICODE_ATTR_ALAC
#define IBSS_ATTR_SHORTEN_DECODED0

#define IRAM_LCDFRAMEBUFFER IBSS_ATTR /* put the lcd frame buffer in IRAM */

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY
