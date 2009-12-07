/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

#define MODEL_NAME   "Archos Player/Studio"

/* define this if you have a charcell LCD display */
#define HAVE_LCD_CHARCELLS

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

#define LCD_WIDTH  11
#define LCD_HEIGHT  2
#define LCD_DEPTH   1
#define SIM_LCD_WIDTH  132 /* pixels */
#define SIM_LCD_HEIGHT  64 /* pixels */

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR       0x000000
#define LCD_BRIGHTCOLOR     0x5a915a
#define LCD_BL_DARKCOLOR    0x000000
#define LCD_BL_BRIGHTCOLOR  0x7ee57e

/* define this if you have the Player's keyboard */
#define CONFIG_KEYPAD PLAYER_PAD

#define AB_REPEAT_ENABLE 1
#define ACTION_WPSAB_SINGLE ACTION_WPS_BROWSE

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x8000

/* Define this if you have a MAS3507D */
#define CONFIG_CODEC MAS3507D

/* Define this if you have a DAC3550A */
#define HAVE_DAC3550A

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define BATTERY_CAPACITY_DEFAULT 1500 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1500 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

#define CURRENT_NORMAL    145  /* usual current in mA */
#define CURRENT_USB       500 /* usual current in mA in USB mode */

/* define this if the unit should not shut down on low battery. */
#define NO_LOW_BATTERY_SHUTDOWN

/* Hardware controlled charging */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* Define this if you have a SH7034 */
#define CONFIG_CPU SH7034

/* Define this if you control ata power player style
   (with PB4, new player only) */
#define ATA_POWER_PLAYERSTYLE

/* Define this to the CPU frequency */
#define CPU_FREQ 12000000 /* cycle time ~83.3ns */

/* Define this if you must discharge the data line by driving it low
   and then set it to input to see if it stays low or goes high */
#define HAVE_I2C_LOW_FIRST

#define CONFIG_I2C I2C_PLAYREC

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 4

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 6

/* The start address index for ROM builds */
#define ROM_START 0x7010 /* for behind BootBox */

/* Software controlled LED */
#define CONFIG_LED LED_REAL

#define CONFIG_LCD LCD_SSD1801

#define BOOTFILE_EXT "mod"
#define BOOTFILE "archos." BOOTFILE_EXT
#define BOOTDIR "/"

#define HAVE_LCD_CONTRAST

#define MIN_CONTRAST_SETTING        5
#define MAX_CONTRAST_SETTING        31
#define DEFAULT_CONTRAST_SETTING    30

#define HAVE_SERIAL

