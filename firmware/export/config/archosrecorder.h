/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

#define MODEL_NAME  "Archos Recorder"

/* define this if you have recording possibility */
#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_SPDIF)

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
/* define this if you have the button bar */
#define HAVE_BUTTONBAR

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  112
#define LCD_HEIGHT 64
#define LCD_DEPTH  1

#define LCD_PIXEL_ASPECT_WIDTH 4
#define LCD_PIXEL_ASPECT_HEIGHT 5

#define LCD_PIXELFORMAT VERTICAL_PACKING

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR       0x000000
#define LCD_BRIGHTCOLOR     0x5a915a
#define LCD_BL_DARKCOLOR    0x000000
#define LCD_BL_BRIGHTCOLOR  0x7ee57e

/* define this if you have the Recorder's 10-key keyboard */
#define CONFIG_KEYPAD RECORDER_PAD

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_M41ST84W

/* define this if you have RTC RAM available for settings */
#define HAVE_RTC_RAM

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x8000

#define AB_REPEAT_ENABLE 1

/* Define this if you have a MAS3587F */
#define CONFIG_CODEC MAS3587F

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define CONFIG_I2C I2C_PLAYREC

#define BATTERY_CAPACITY_DEFAULT 1500 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1500 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

if MEM < 8
 #define CURRENT_NORMAL    145  /* usual current in mA */
#else
 #define CURRENT_NORMAL    145 *100 / 122 /* assuming 192 kbps, the running time is 22% longer with 8MB */
#endif
#define CURRENT_RECORD     35  /* additional recording current */
#define CURRENT_USB       500 /* usual current in mA in USB mode */

/* define this if the unit should not shut down on low battery. */
#define NO_LOW_BATTERY_SHUTDOWN

/* Software controlled charging */
#define CONFIG_CHARGING CHARGING_TARGET

/* Define this if you have a SH7034 */
#define CONFIG_CPU SH7034

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Define this to the CPU frequency */
#define CPU_FREQ      11059200

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 4

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 6

/* The start address index for ROM builds */
/* #define ROM_START 0x11010 for behind original Archos */
#define ROM_START 0x7010 /* for behind BootBox */

/* Software controlled LED */
#define CONFIG_LED LED_REAL

/* Define this for S/PDIF output available */
#define HAVE_SPDIF_OUT

#define CONFIG_LCD LCD_SSD1815

#define BOOTFILE_EXT "ajz"
#define BOOTFILE "ajbrec." BOOTFILE_EXT
#define BOOTDIR "/"

#define HAVE_LCD_CONTRAST

#define MIN_CONTRAST_SETTING        5
#define MAX_CONTRAST_SETTING        63

#define HAVE_SERIAL
