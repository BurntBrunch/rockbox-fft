/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Pacbox - a Pacman Emulator for Rockbox
 *
 * Based on PIE - Pacman Instructional Emulator
 *
 * Copyright (c) 1997-2003,2004 Alessandro Scotti
 * http://www.ascotti.org/
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

#include "plugin.h"
#include "arcade.h"
#include "pacbox.h"
#include "pacbox_lcd.h"
#include "lib/configfile.h"
#include "lib/playback_control.h"

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

struct pacman_settings {
    int difficulty;
    int numlives;
    int bonus;
    int ghostnames;
    int showfps;
};

static struct pacman_settings settings;
static struct pacman_settings old_settings;

#define SETTINGS_VERSION 1
#define SETTINGS_MIN_VERSION 1
#define SETTINGS_FILENAME "pacbox.cfg"

static char* difficulty_options[] = { "Normal", "Hard" };
static char* numlives_options[] = { "1", "2", "3", "5" };
static char* bonus_options[] = {"10000", "15000", "20000", "No Bonus"};
static char* ghostnames_options[] = {"Normal", "Alternate"};
static char* showfps_options[] = {"No", "Yes"};

static struct configdata config[] =
{
   {TYPE_ENUM, 0, 2, { .int_p = &settings.difficulty }, "Difficulty",
    difficulty_options},
   {TYPE_ENUM, 0, 4, { .int_p = &settings.numlives }, "Pacmen Per Game",
    numlives_options},
   {TYPE_ENUM, 0, 4, { .int_p = &settings.bonus }, "Bonus", bonus_options},
   {TYPE_ENUM, 0, 2, { .int_p = &settings.ghostnames }, "Ghost Names",
    ghostnames_options},
   {TYPE_ENUM, 0, 2, { .int_p = &settings.showfps }, "Show FPS",
    showfps_options},
};

static bool loadFile( const char * name, unsigned char * buf, int len )
{
    char filename[MAX_PATH];

    rb->snprintf(filename,sizeof(filename), ROCKBOX_DIR "/pacman/%s",name);

    int fd = rb->open( filename, O_RDONLY);

    if( fd < 0 ) {
        return false;
    }

    int n = rb->read( fd, buf, len);

    rb->close( fd );

    if( n != len ) {
        return false;
    }

    return true;
}

static bool loadROMS( void )
{
    bool romsLoaded = false;

    romsLoaded = loadFile( "pacman.6e", ram_,           0x1000) &&
                 loadFile( "pacman.6f", ram_+0x1000,    0x1000) &&
                 loadFile( "pacman.6h", ram_+0x2000,    0x1000) &&
                 loadFile( "pacman.6j", ram_+0x3000,    0x1000) &&
                 loadFile( "pacman.5e", charset_rom_,   0x1000) &&
                 loadFile( "pacman.5f", spriteset_rom_, 0x1000);

    if( romsLoaded ) {
        decodeROMs();
        reset_PacmanMachine();
    }

    return romsLoaded;
}

/* A buffer to render Pacman's 244x288 screen into */
static unsigned char video_buffer[ScreenWidth*ScreenHeight] __attribute__ ((aligned (16)));

static long start_time;
static long video_frames = 0;

static int dipDifficulty[] = { DipDifficulty_Normal, DipDifficulty_Hard };
static int dipLives[] = { DipLives_1, DipLives_2, DipLives_3, DipLives_5 };
static int dipBonus[] = { DipBonus_10000, DipBonus_15000, DipBonus_20000, 
                          DipBonus_None };
static int dipGhostNames[] = { DipGhostNames_Normal, DipGhostNames_Alternate };

static int settings_to_dip(struct pacman_settings settings)
{
    return ( DipPlay_OneCoinOneGame | 
             DipCabinet_Upright | 
             DipMode_Play |
             DipRackAdvance_Off |

             dipDifficulty[settings.difficulty] |
             dipLives[settings.numlives] |
             dipBonus[settings.bonus] |
             dipGhostNames[settings.ghostnames]
           );
}

static bool pacbox_menu(void)
{
    int selected=0;
    int result;
    int menu_quit=0;
    int new_setting;
    bool need_restart = false;

    static const struct opt_items noyes[2] = {
        { "No", -1 },
        { "Yes", -1 },
    };

    static const struct opt_items difficulty_options[2] = {
        { "Normal", -1 },
        { "Harder", -1 },
    };

    static const struct opt_items numlives_options[4] = {
        { "1", -1 },
        { "2", -1 },
        { "3", -1 },
        { "5", -1 },
    };

    static const struct opt_items bonus_options[4] = {
        { "10000 points", -1 },
        { "15000 points", -1 },
        { "20000 points", -1 },
        { "No bonus", -1 },
    };

    static const struct opt_items ghostname_options[2] = {
        { "Normal", -1 },
        { "Alternate", -1 },
    };

    MENUITEM_STRINGLIST(menu, "Pacbox Menu", NULL,
                        "Difficulty", "Pacmen Per Game", "Bonus Life",
                        "Ghost Names", "Display FPS",
                        "Restart", "Quit");

    rb->button_clear_queue();
    
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_RGB565);
#endif

    while (!menu_quit) {
        result=rb->do_menu(&menu, &selected, NULL, false);

        switch(result)
        {
            case 0:
                new_setting=settings.difficulty;
                rb->set_option("Difficulty", &new_setting, INT,
                               difficulty_options , 2, NULL);
                if (new_setting != settings.difficulty) {
                    settings.difficulty=new_setting;
                    need_restart=true;
                }
                break;
            case 1:
                new_setting=settings.numlives;
                rb->set_option("Pacmen Per Game", &new_setting, INT,
                               numlives_options , 4, NULL);
                if (new_setting != settings.numlives) {
                    settings.numlives=new_setting;
                    need_restart=true;
                }
                break;
            case 2:
                new_setting=settings.bonus;
                rb->set_option("Bonus Life", &new_setting, INT,
                               bonus_options , 4, NULL);
                if (new_setting != settings.bonus) {
                    settings.bonus=new_setting;
                    need_restart=true;
                }
                break;
            case 3:
                new_setting=settings.ghostnames;
                rb->set_option("Ghost Names", &new_setting, INT,
                               ghostname_options , 2, NULL);
                if (new_setting != settings.ghostnames) {
                    settings.ghostnames=new_setting;
                    need_restart=true;
                }
                break;
            case 4: /* Show FPS */
                rb->set_option("Display FPS",&settings.showfps,INT,
                               noyes, 2, NULL);
                break;
            case 5: /* Restart */
                need_restart=true;
                menu_quit=1;
                break;
            default:
                menu_quit=1;
                break;
        }
    }
    
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_PAL256);
#endif

    if (need_restart) {
        init_PacmanMachine(settings_to_dip(settings));
    }

    /* Possible results: 
         exit game
         restart game
         usb connected
    */
    return (result==6);
}


/*
    Runs the game engine for one frame.
*/
static int gameProc( void )
{
    int x;
    int fps;
    char str[80];
    int status;
    long end_time;
    int frame_counter = 0;
    int yield_counter = 0;

    while (1)
    {
        /* Run the machine for one frame (1/60th second) */
        run();

        frame_counter++;

        /* Check the button status */
        status = rb->button_status();

#ifdef HAS_BUTTON_HOLD
        if (rb->button_hold())
        status = PACMAN_MENU;
#endif

        if ((status & PACMAN_MENU) == PACMAN_MENU
#ifdef PACMAN_RC_MENU
            || status == PACMAN_RC_MENU
#endif
        ) {
            end_time = *rb->current_tick;
            x = pacbox_menu();
            rb->lcd_clear_display();
#ifdef HAVE_REMOTE_LCD
            rb->lcd_remote_clear_display();
            rb->lcd_remote_update();
#endif
            if (x == 1) { return 1; }
            start_time += *rb->current_tick-end_time;
        }

#ifdef PACMAN_HAS_REMOTE
        setDeviceMode( Joy1_Left, (status & PACMAN_LEFT || status == PACMAN_RC_LEFT) ? DeviceOn : DeviceOff);
        setDeviceMode( Joy1_Right, (status & PACMAN_RIGHT || status == PACMAN_RC_RIGHT) ? DeviceOn : DeviceOff);
        setDeviceMode( Joy1_Up, (status & PACMAN_UP || status == PACMAN_RC_UP) ? DeviceOn : DeviceOff);
        setDeviceMode( Joy1_Down, (status & PACMAN_DOWN || status == PACMAN_RC_DOWN) ? DeviceOn : DeviceOff);
        setDeviceMode( CoinSlot_1, (status & PACMAN_COIN || status == PACMAN_RC_COIN) ? DeviceOn : DeviceOff);
        setDeviceMode( Key_OnePlayer, (status & PACMAN_1UP || status == PACMAN_RC_1UP) ? DeviceOn : DeviceOff);
        setDeviceMode( Key_TwoPlayers, (status & PACMAN_2UP || status == PACMAN_RC_2UP) ? DeviceOn : DeviceOff);
#else
        setDeviceMode( Joy1_Left, (status & PACMAN_LEFT) ? DeviceOn : DeviceOff);
        setDeviceMode( Joy1_Right, (status & PACMAN_RIGHT) ? DeviceOn : DeviceOff);
        setDeviceMode( Joy1_Up, (status & PACMAN_UP) ? DeviceOn : DeviceOff);
        setDeviceMode( Joy1_Down, (status & PACMAN_DOWN) ? DeviceOn : DeviceOff);
        setDeviceMode( CoinSlot_1, (status & PACMAN_COIN) ? DeviceOn : DeviceOff);
        setDeviceMode( Key_OnePlayer, (status & PACMAN_1UP) ? DeviceOn : DeviceOff);
#ifdef PACMAN_2UP
        setDeviceMode( Key_TwoPlayers, (status & PACMAN_2UP) ? DeviceOn : DeviceOff);
#endif
#endif

        /* We only update the screen every third frame - Pacman's native 
           framerate is 60fps, so we are attempting to display 20fps */
        if (frame_counter == 60 / FPS) {

            frame_counter = 0;
            video_frames++;

            yield_counter ++;

            if (yield_counter == FPS) {
                yield_counter = 0;
                rb->yield ();
            }

            /* The following functions render the Pacman screen from the 
               contents of the video and color ram.  We first update the 
               background, and then draw the Sprites on top. 
            */

            renderBackground( video_buffer );
            renderSprites( video_buffer );

#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
            rb->lcd_blit_pal256(    video_buffer, 0, 0, XOFS, YOFS, 
                                    ScreenWidth, ScreenHeight);
#else
            blit_display(rb->lcd_framebuffer,video_buffer);
#endif

            if (settings.showfps) {
                fps = (video_frames*HZ*100) / (*rb->current_tick-start_time);
                rb->snprintf(str,sizeof(str),"%d.%02d / %d fps  ",
                                             fps/100,fps%100,FPS);
                rb->lcd_putsxy(0,0,str);
            }

#if !defined(HAVE_LCD_MODES) || \
    defined(HAVE_LCD_MODES) && !(HAVE_LCD_MODES & LCD_MODE_PAL256)
            rb->lcd_update();
#endif

            /* Keep the framerate at Pacman's 60fps */
            end_time = start_time + (video_frames*HZ)/FPS;
            while (TIME_BEFORE(*rb->current_tick,end_time)) {
                rb->sleep(1);
            }
        }
    }
    return 0;
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    PLUGIN_IRAM_INIT(rb)

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_clear_display();
    rb->lcd_update();

    /* Set the default settings */
    settings.difficulty = 0; /* Normal */
    settings.numlives = 2;   /* 3 lives */
    settings.bonus = 0;      /* 10000 points */
    settings.ghostnames = 0; /* Normal names */
    settings.showfps = 0;    /* Do not show FPS */

    if (configfile_load(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_MIN_VERSION
                       ) < 0)
    {
        /* If the loading failed, save a new config file (as the disk is
           already spinning) */
        configfile_save(SETTINGS_FILENAME, config,
                        sizeof(config)/sizeof(*config),
                        SETTINGS_VERSION);
    }

    /* Keep a copy of the saved version of the settings - so we can check if 
       the settings have changed when we quit */
    old_settings = settings;

    /* Initialise the hardware */
    init_PacmanMachine(settings_to_dip(settings));
    
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_PAL256);
#endif

    /* Load the romset */
    if (loadROMS()) {
        start_time = *rb->current_tick-1;

        gameProc();

        /* Save the user settings if they have changed */
        if (rb->memcmp(&settings,&old_settings,sizeof(settings))!=0) {
            rb->splash(0, "Saving settings...");
            configfile_save(SETTINGS_FILENAME, config,
                            sizeof(config)/sizeof(*config),
                            SETTINGS_VERSION);
        }
    } else {
        rb->splashf(HZ*2, "No ROMs in %s/pacman/", ROCKBOX_DIR);
    }
    
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_RGB565);
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    return PLUGIN_OK;
}
