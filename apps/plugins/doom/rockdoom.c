/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 Karl Kurbjun
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 * 
 * H300 Port by Karl Kurbjun
 * IPod port by Dave Chapman and Paul Louden
 * Additional code contributed by Thom Johansen
 * Based off work by:   Digita Doom, IDoom, Prboom, lSDLDoom, LxDoom,
 *                      MBF, Boom, DosDoom,
 *                      and of course Original Doom by ID Software
 * See: http://prboom.sourceforge.net/about.html for the history
 *
 *
 ****************************************************************************/

#include "d_main.h"
#include "doomdef.h"
#include "settings.h"
#include "m_fixed.h"
#include "m_argv.h"
#include "m_misc.h"
#include "g_game.h"
#include "rockmacros.h"
#include "doomstat.h"
#include "i_system.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "lib/helper.h"

PLUGIN_HEADER
PLUGIN_IRAM_DECLARE

extern boolean timingdemo, singledemo, demoplayback, fastdemo; // killough

int filearray[9];
int fpoint=1; // save 0 for closing

int fileexists(const char * fname)
{
   int fd;
   fd = open(fname,O_RDONLY);

   if (fd>=0)
   {
      close(fd);
      return 0;
   }
   return -1;
}

#ifndef SIMULATOR
int my_open(const char *file, int flags)
{
   if(fpoint==8)
      return -1;
#undef open
   filearray[fpoint]=rb->open(file, flags);

   if(filearray[fpoint]<0)
      return filearray[fpoint];

   fpoint++;
   return filearray[fpoint-1];
}

int my_close(int id)
{
   int i=0;
   if(id<0)
      return id;
   while(filearray[i]!=id && i<8)
      i++;

   if(i==8)
   {
      printf("A requested FID did not exist!!!!");
      return -9;
   }
#undef close
   rb->close(id);

   for(; i<fpoint-1; i++)
      filearray[i]=filearray[i+1];

   fpoint--;
   return 0;
}
#endif
#define MAXARGVS  100

bool noprintf=0;  // Variable disables printf lcd updates to protect grayscale lib/direct lcd updates

#ifndef SIMULATOR
// Here is a hacked up printf command to get the output from the game.
int printf(const char *fmt, ...)
{
   static int p_xtpt;
   char p_buf[50];
   bool ok;
   rb->yield();
   va_list ap;

   va_start(ap, fmt);
   ok = vsnprintf(p_buf,sizeof(p_buf), fmt, ap);
   va_end(ap);

   rb->lcd_putsxy(1,p_xtpt, (unsigned char *)p_buf);
   if (!noprintf)
      rb->lcd_update();

   p_xtpt+=8;
   if(p_xtpt>LCD_HEIGHT-8)
   {
      p_xtpt=0;
      if (!noprintf)
         rb->lcd_clear_display();
   }
   return 1;
}
#endif

char *my_strtok( char * s, const char * delim )
{
    register char *spanp;
    register int c, sc;
    char *tok;
   static char *lasts;


    if (s == NULL && (s = lasts) == NULL)
        return (NULL);

    /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc)
            goto cont;
    }

    if (c == 0) {        /* no non-delimiter characters */
        lasts = NULL;
        return (NULL);
    }
    tok = s - 1;

    /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;) {
        c = *s++;
        spanp = (char *)delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                lasts = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}

struct argvlist
{
   int timedemo;        // 1 says there's a timedemo
   int demonum;
   int addonnum;
} argvlist;

const unsigned char versions_builtin[7][20] =
{
   "Doom Shareware",
   "Doom Registered",
   "Ultimate Doom",
   "Doom 2",
   "Freedoom",
   "Plutonia",
   "TNT"
};

const unsigned char wads_builtin[7][30] =
{
   GAMEBASE"doom1.wad",
   GAMEBASE"doom.wad",
   GAMEBASE"doomu.wad",
   GAMEBASE"doom2.wad",
   GAMEBASE"doomf.wad",
   GAMEBASE"plutonia.wad",
   GAMEBASE"tnt.wad"
};

int namemap[7];
static char **addons;
static char **demolmp;
char addon[200];
// This sets up the base game and builds up myargv/c
bool Dhandle_ver (int dver)
{
   switch (dver) {
      case 0: /* Doom Shareware */
         gamemode = shareware;
         gamemission = doom;
         D_AddFile(wads_builtin[0],source_iwad);
         break;
      case 1: /* Doom registered */
         gamemode = registered;
         gamemission = doom;
         D_AddFile(wads_builtin[1],source_iwad);
         break;
      case 2: /* Ultimate Doom */
         gamemode = retail;
         gamemission = doom;
         D_AddFile(wads_builtin[2],source_iwad);
         break;
      case 3: /* Doom2 */
         gamemode = commercial;
         gamemission = doom2;
         D_AddFile(wads_builtin[3],source_iwad);
         break;
      case 4: /* Doom2f */
         gamemode = commercial;
         gamemission = doom2;
         D_AddFile(wads_builtin[4],source_iwad);
         break;
      case 5: /* Plutonia */
         gamemode = commercial;
         gamemission = pack_plut;
         D_AddFile(wads_builtin[5],source_iwad);
         break;
      case 6: /* TNT */
         gamemode = commercial;
         gamemission = pack_tnt;
         D_AddFile(wads_builtin[6],source_iwad);
         break;
      default:
         gamemission = none;
         return 0;
   }
   // Start adding to myargv
   if(argvlist.timedemo && (gamemode == shareware))
   {
         singletics = true;
         timingdemo = true;            // show stats after quit
         G_DeferedPlayDemo("demo3");
         singledemo = true;            // quit after one demo
   }

   if(argvlist.addonnum)
   {
      snprintf(addon,sizeof(addon),"%s%s", GAMEBASE"addons/", addons[argvlist.addonnum]);
      D_AddFile(addon,source_pwad);
      modifiedgame = true; 
   }

   if(argvlist.demonum)
   {
      snprintf(addon, sizeof(addon),"%s%s", GAMEBASE"demos/", demolmp[argvlist.demonum]);
      D_AddFile(addon, source_lmp);
      G_DeferedPlayDemo(addon);
      singledemo = true;          // quit after one demo
   }
   return 1;
}

// This function builds up the basegame list for use in the options selection
// it also sets the defaults for the argvlist
// Now checking for rcokdoom.wad based on prboom.wad
int Dbuild_base (struct opt_items *names)
{
   if ( fileexists(GAMEBASE"rockdoom.wad") )
      return 0;

   D_AddFile (GAMEBASE"rockdoom.wad", source_pwad);

   int i=0, j;
   /* Doom Shareware */
   /* Doom registered */
   /* Ultimate Doom */
   /* Doom2 */
   /* Doom2f */
   /* Plutonia */
   /* TNT */
    for(j=0;j<7;j++)
        if ( !fileexists (wads_builtin[j]) )
        {
            names[i].string=versions_builtin[j];
            names[i].voice_id=-1;
            namemap[i]=j;
            i++;
        }
   // Set argvlist defaults
   argvlist.timedemo=0;

   return i;
}

// This is a general function that takes in a menu_item structure and makes a list
// of files within it based on matching the string stringmatch to the files.
int Dbuild_filelistm(char ***names, char *firstentry, char *directory, char *stringmatch)
{
   int            i=0;
   DIR            *filedir;
   struct dirent  *dptr;
   char           *startpt;
   char           **temp;

   filedir=rb->opendir(directory);

   if(filedir==NULL)
   {
      temp=malloc(sizeof(char *));
      temp[0]=firstentry;
      *names=temp;
      return 1;
   }

   // Get the total number of entries
   while((dptr=rb->readdir(filedir)))
      i++;

   // Reset the directory
   rb->closedir(filedir);
   filedir=rb->opendir(directory);

   i++;
   temp=malloc(i*sizeof(char *));
   temp[0]=firstentry;
   i=1;

   while((dptr=rb->readdir(filedir)))
   {
      if(rb->strcasestr(dptr->d_name, stringmatch))
      {
         startpt=malloc(strlen(dptr->d_name)*sizeof(char));
         strcpy(startpt,dptr->d_name);
         temp[i]=startpt;
         i++;
      }
   }
   rb->closedir(filedir);
   *names=temp;
   return i;
}

static int translatekey(int key) __attribute__ ((noinline));

// This key configuration code is not the cleanest or the most efficient, but it works
static int translatekey(int key)
{
   if (key<31)
   {
      switch(key)
      {
         case 0:
            return 0;
         case 1:
            return KEY_RIGHTARROW;
         case 2:
            return KEY_LEFTARROW;
         case 3:
            return KEY_UPARROW;
         case 4:
            return KEY_DOWNARROW;
         case 5:
            return KEY_ENTER;
         case 6:
            return KEY_RCTRL;
         case 7:
            return ' ';
         case 8:
            return KEY_ESCAPE;
         case 9:
            return 'w';
         case 10:
            return KEY_TAB;
         default:
            return 0;
      }
   }
   else
   {
      switch(key)
      {
         case 0:
            return 0;
         case KEY_RIGHTARROW:
            return 1;
         case KEY_LEFTARROW:
            return 2;
         case KEY_UPARROW:
            return 3;
         case KEY_DOWNARROW:
            return 4;
         case KEY_ENTER:
            return 5;
         case KEY_RCTRL:
            return 6;
         case ' ':
            return 7;
         case KEY_ESCAPE:
            return 8;
         case 'w':
            return 9;
         case KEY_TAB:
            return 10;
         default:
            return 0;
      }
   }
}

// I havn't added configurable keys for enter or escape because this requires some modification to
// m_menu.c which hasn't been done yet.

int Oset_keys()
{
   int selected=0, result;
   int menuquit=0;


   static const struct opt_items doomkeys[] = {
      { "Unmapped", -1 },
      { "Key Right", -1 },
      { "Key Left", -1 },
      { "Key Up", -1 },
      { "Key Down", -1 },
      { "Key Select", -1 },
#if defined(TOSHIBA_GIGABEAT_F)
      { "Key A", -1 },
      { "Key Menu", -1 },
      { "Key Power", -1 },
      { "Key Volume Down", -1 },
      { "Key Volume Up", -1 },
#else
      { "Key Record", -1 },
      { "Key Mode", -1 },
      { "Key Off", -1 },
      { "Key On", -1 },
#endif
   };
   
    int *keys[]={
        &key_right,
        &key_left,
        &key_up,
        &key_down,
        &key_fire,
        &key_use,
        &key_strafe,
        &key_weapon,
        &key_map
    };

   int numdoomkeys=sizeof(doomkeys) / sizeof(*doomkeys);

    MENUITEM_STRINGLIST(menu, "Set Keys", NULL,
                        "Game Right", "Game Left", "Game Up", "Game Down",
                        "Game Shoot", "Game Open", "Game Strafe",
                        "Game Weapon", "Game Automap");

    while(!menuquit)
    {
        result = rb->do_menu(&menu, &selected, NULL, false);
        if(result<0)
            menuquit=1;
        else
        {
            *keys[result]=translatekey(*keys[result]);
            rb->set_option(menu_[result], keys[result], INT, doomkeys, numdoomkeys, NULL );
            *keys[result]=translatekey(*keys[result]);
        }
   }

   return (1);
}

extern int fake_contrast;

static bool Doptions()
{
   static const struct opt_items onoff[2] = {
      { "Off", -1 },
      { "On", -1 },
   };

   int selected=0, result;
   int menuquit=0;

    MENUITEM_STRINGLIST(menu, "Options", NULL,
                        "Set Keys", "Sound", "Timedemo", "Player Bobbing",
                        "Weapon Recoil", "Translucency", "Fake Contrast",
                        "Always Run", "Headsup Display", "Statusbar Always Red",
#if(LCD_HEIGHT>LCD_WIDTH)
                        "Rotate Screen 90 deg",
#endif
                        );
   
   void *options[]={
        &enable_sound,
        &argvlist.timedemo,
        &default_player_bobbing,
        &default_weapon_recoil,
        &default_translucency,
        &fake_contrast,
        &autorun,
        &hud_displayed,
        &sts_always_red,
#if(LCD_HEIGHT>LCD_WIDTH)
        &rotate_screen,
#endif
    };

    while(!menuquit)
    {
        result = rb->do_menu(&menu, &selected, NULL, false);
        if(result==0) 
            Oset_keys();
        else if (result > 0)
            rb->set_option(menu_[result], options[result-1], INT, onoff, 2, NULL );
        else
            menuquit=1;
    }

    return (1);
}

static const char* choice_get_name(int selected_item, void * data,
                                   char * buffer, size_t buffer_len)
{
    const char **names = (const char **) data;
    (void) buffer;
    (void) buffer_len;
    return names[selected_item];
}

int list_action_callback(int action, struct gui_synclist *lists)
{
    (void) lists;
    if (action == ACTION_STD_OK)
        return ACTION_STD_CANCEL;
    return action;
}

bool menuchoice(char **names, int count, int *selected)
{
   struct simplelist_info info;
   rb->simplelist_info_init(&info, NULL, count, (void*)names);
   info.selection = *selected;
   info.get_name = choice_get_name;
   info.action_callback = list_action_callback;
   if(rb->simplelist_show_list(&info))
      return true;

   if(info.selection<count && info.selection>=0)
      *selected = info.selection;
   return false;
}

//
// Doom Menu
//
int doom_menu()
{
   int selected=0, result;
   int status;
   int gamever;
   bool menuquit=0;

   static struct opt_items names[7];

   MENUITEM_STRINGLIST(menu, "Doom Menu", NULL,
                       "Game", "Addons", "Demos",
                       "Options", "Play Game", "Quit");

   if( (status=Dbuild_base(names)) == 0 ) // Build up the base wad files (select last added file)
   {
      rb->splash(HZ*2, "Missing Base WAD!");
      return -2;
   }

   int numadd=Dbuild_filelistm(&addons, "No Addon", GAMEBASE"addons/", ".WAD" );

   int numdemos=Dbuild_filelistm(&demolmp, "No Demo", GAMEBASE"demos/", ".LMP" );

   argvlist.demonum=0;
   argvlist.addonnum=0;

   gamever=status-1;

    /* Clean out the button Queue */
    while (rb->button_get(false) != BUTTON_NONE) 
        rb->yield();

   while(!menuquit)
   {
      result = rb->do_menu(&menu, &selected, NULL, false);
      switch (result) {
         case 0: /* Game picker */
            rb->set_option("Game WAD", &gamever, INT, names, status, NULL );
            break;

         case 1: /* Addon picker */
            menuchoice(addons,numadd,&argvlist.addonnum);
            break;

         case 2: /* Demos */
            menuchoice(demolmp,numdemos,&argvlist.demonum);
            break;

         case 3: /* Options */
            Doptions();
            break;

         case 4: /* Play Game */
            menuquit=1;
            break;

         case 5: /* Quit */
            menuquit=1;
            gamever=-1;
            break;

         default:
            break;
      }
   }

   return (gamever);
}

extern int systemvol;
/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
   /* Disable all talking before initializing IRAM */
   rb->talk_disable(true);

   PLUGIN_IRAM_INIT(rb)

   (void)parameter;

   doomexit=0;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
   rb->cpu_boost(true);
#endif

   rb->lcd_setfont(0);

   // We're using doom's memory management since it implements a proper free (and re-uses the memory)
   // and now with prboom's code: realloc and calloc
   printf ("Z_Init: Init zone memory allocation daemon.\n");
   Z_Init ();

   printf ("M_LoadDefaults: Load system defaults.\n");
   M_LoadDefaults ();              // load before initing other systems

   rb->splash(HZ*2, "Welcome to RockDoom");

   myargv =0;
   myargc=0;

   rb->lcd_clear_display();

   int result = doom_menu();
   if (result < 0)
   {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
       rb->cpu_boost(false);
#endif
       rb->talk_disable(false);
       if( result == -1 )
           return PLUGIN_OK; // Quit was selected
       else
           return PLUGIN_ERROR; // Missing base wads
   }

#if(LCD_HEIGHT>LCD_WIDTH)
    if(rotate_screen)
    {
        SCREENHEIGHT=LCD_WIDTH;
        SCREENWIDTH=LCD_HEIGHT;
    }
    else
    {
        SCREENHEIGHT=LCD_HEIGHT;
        SCREENWIDTH=LCD_WIDTH;
    }
#endif

   Dhandle_ver( namemap[ result ] );

   rb->lcd_setfont(0);

   rb->lcd_clear_display();

   int mod = (rb->sound_max(SOUND_VOLUME)-rb->sound_min(SOUND_VOLUME))/15;
   if(mod == 0)
       mod = rb->global_settings->volume;
   systemvol= rb->global_settings->volume-rb->global_settings->volume%mod;
   general_translucency = default_translucency;                    // phares

   backlight_force_on();
#ifdef RB_PROFILE
   rb->profile_thread();
#endif

#if LCD_DEPTH>1
   rb->lcd_set_backdrop(NULL);
#endif

   D_DoomMain ();

#ifdef RB_PROFILE
   rb->profstop();
#endif
   backlight_use_settings();

   M_SaveDefaults ();

   I_Quit(); // Make SURE everything was closed out right

   printf("There were still: %d files open\n", fpoint);
   while(fpoint>0)
   {
#ifdef SIMULATOR
      close(filearray[fpoint]);
#else
      rb->close(filearray[fpoint]);
#endif
      fpoint--;
   }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
   rb->cpu_boost(false);
#endif

   rb->talk_disable(false);
   return PLUGIN_OK;
}
