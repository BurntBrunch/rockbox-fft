/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "string.h"
#include <ctype.h>

#include "sprintf.h"
#include "settings.h"
#include "debug.h"
#include "lang.h"
#include "kernel.h"
#include "plugin.h"
#include "filetypes.h"
#include "screens.h"
#include "dir.h"
#include "file.h"
#include "splash.h"
#include "buffer.h"
#include "icons.h"
#include "logf.h"

/* max filetypes (plugins & icons stored here) */
#if CONFIG_CODEC == SWCODEC
#define MAX_FILETYPES 128
#else
#define MAX_FILETYPES 64
#endif

/* a table for the know file types */
static const struct filetype inbuilt_filetypes[] = {
    { "mp3", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mp2", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mpa", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
#if CONFIG_CODEC == SWCODEC
    /* Temporary hack to allow playlist creation */
    { "mp1", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "ogg", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "oga", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "wma", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "wmv", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "asf", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "wav", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "flac",FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "ac3", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "a52", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mpc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "wv",  FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "m4a", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "m4b", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mp4", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mod", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "shn", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "aif", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "aiff",FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "spx" ,FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "sid", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "adx", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "nsf", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "nsfe",FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "spc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "ape", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mac", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "sap" ,FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "rm",  FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "ra",  FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "rmvb",FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "cmc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "cm3", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "cmr", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "cms", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "dmc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "dlt", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mpt", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mpd", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "rmt", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "tmc", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "tm8", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "tm2", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "oma", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "aa3", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "at3", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
    { "mmf", FILE_ATTR_AUDIO, Icon_Audio, VOICE_EXT_MPA },
#endif
    { "m3u", FILE_ATTR_M3U, Icon_Playlist, LANG_PLAYLIST },
    { "m3u8",FILE_ATTR_M3U, Icon_Playlist, LANG_PLAYLIST },
    { "cfg", FILE_ATTR_CFG, Icon_Config,   VOICE_EXT_CFG },
    { "wps", FILE_ATTR_WPS, Icon_Wps,      VOICE_EXT_WPS },
#ifdef HAVE_REMOTE_LCD
    { "rwps",FILE_ATTR_RWPS, Icon_Wps,     VOICE_EXT_RWPS },
#endif
#if CONFIG_TUNER
    { "fmr", FILE_ATTR_FMR, Icon_Preset, LANG_FMR },
#endif
    { "lng", FILE_ATTR_LNG, Icon_Language, LANG_LANGUAGE },
    { "rock",FILE_ATTR_ROCK,Icon_Plugin,   VOICE_EXT_ROCK },
    { "lua", FILE_ATTR_LUA, Icon_Plugin,   VOICE_EXT_ROCK },
#ifdef HAVE_LCD_BITMAP
    { "fnt", FILE_ATTR_FONT,Icon_Font,     VOICE_EXT_FONT },
    { "kbd", FILE_ATTR_KBD, Icon_Keyboard, VOICE_EXT_KBD },
#endif
    { "bmark",FILE_ATTR_BMARK, Icon_Bookmark,  VOICE_EXT_BMARK },
    { "cue",  FILE_ATTR_CUE,   Icon_Bookmark,  VOICE_EXT_CUESHEET },
#ifdef HAVE_LCD_BITMAP
    { "sbs",  FILE_ATTR_SBS,  Icon_Wps,   VOICE_EXT_SBS },
#endif
#ifdef HAVE_REMOTE_LCD
    { "rsbs", FILE_ATTR_RSBS, Icon_Wps,   VOICE_EXT_RSBS },
#endif
#ifdef BOOTFILE_EXT
    { BOOTFILE_EXT, FILE_ATTR_MOD, Icon_Firmware, VOICE_EXT_AJZ },
#endif
#ifdef BOOTFILE_EXT2
    { BOOTFILE_EXT2, FILE_ATTR_MOD, Icon_Firmware, VOICE_EXT_AJZ },
#endif
};

void tree_get_filetypes(const struct filetype** types, int* count)
{
    *types = inbuilt_filetypes;
    *count = sizeof(inbuilt_filetypes) / sizeof(*inbuilt_filetypes);
}

/* mask for dynamic filetype info in attribute */
#define ROCK_EXTENSION "rock"

struct file_type {
    int  icon; /* the icon which shall be used for it, NOICON if unknown */
    unsigned char  attr; /* FILE_ATTR_MASK >> 8 */
    char* plugin; /* Which plugin to use, NULL if unknown, or builtin */
    char* extension; /* NULL for none */
};
static struct file_type filetypes[MAX_FILETYPES];
static int custom_filetype_icons[MAX_FILETYPES];
static bool custom_icons_loaded = false;
#ifdef HAVE_LCD_COLOR
static int custom_colors[MAX_FILETYPES+1];
#endif
static int filetype_count = 0;
static unsigned char highest_attr = 0;

static char *filetypes_strdup(char* string)
{
    char *buffer = (char*)buffer_alloc(strlen(string)+1);
    strcpy(buffer, string);
    return buffer;
}
static void read_builtin_types(void);
static void read_config(char* config_file);
#ifdef HAVE_LCD_COLOR
/* Colors file format is similar to icons:
 * ext:hex_color
 * load a colors file from a theme with:
 * filetype colours: filename.colours */
void read_color_theme_file(void) {
    char buffer[MAX_PATH];
    int fd;
    char *ext, *color;
    int i;
    for (i = 0; i < MAX_FILETYPES+1; i++) {
        custom_colors[i] = -1;
    }
    snprintf(buffer, MAX_PATH, "%s/%s.colours", THEME_DIR, 
             global_settings.colors_file);
    fd = open(buffer, O_RDONLY);
    if (fd < 0)
        return;
    while (read_line(fd, buffer, MAX_PATH) > 0)
    {
        if (!settings_parseline(buffer, &ext, &color))
            continue;
        if (!strcasecmp(ext, "folder"))
        {
            hex_to_rgb(color, &custom_colors[0]);
            continue;
        }
        if (!strcasecmp(ext, "???"))
        {
            hex_to_rgb(color, &custom_colors[MAX_FILETYPES]);
            continue;
        }
        for (i=1; i<filetype_count; i++)
        {
            if (filetypes[i].extension &&
                !strcasecmp(ext, filetypes[i].extension))
            {
                hex_to_rgb(color, &custom_colors[i]);
                break;
            }
        }
    }
    close(fd);
}
#endif
#ifdef HAVE_LCD_BITMAP
void read_viewer_theme_file(void)
{
    char buffer[MAX_PATH];
    int fd;
    char *ext, *icon;
    int i;
    global_status.viewer_icon_count = 0;
    custom_icons_loaded = false;
    custom_filetype_icons[0] = Icon_Folder;
    for (i=1; i<filetype_count; i++)
    {
        custom_filetype_icons[i] = filetypes[i].icon;
    }
    
    snprintf(buffer, MAX_PATH, "%s/%s.icons", ICON_DIR, 
             global_settings.viewers_icon_file);
    fd = open(buffer, O_RDONLY);
    if (fd < 0)
        return;
    while (read_line(fd, buffer, MAX_PATH) > 0)
    {
        if (!settings_parseline(buffer, &ext, &icon))
            continue;
        for (i=0; i<filetype_count; i++)
        {
            if (filetypes[i].extension &&
                !strcasecmp(ext, filetypes[i].extension))
            {
                if (*icon == '*')
                    custom_filetype_icons[i] = atoi(icon+1);
                else if (*icon == '-')
                    custom_filetype_icons[i] = Icon_NOICON;
                else if (*icon >= '0' && *icon <= '9')
                {
                    int number = atoi(icon);
                    if (number > global_status.viewer_icon_count)
                        global_status.viewer_icon_count++;
                    custom_filetype_icons[i] = Icon_Last_Themeable + number;
                }
                break;
            }
        }
    }
    close(fd);
    custom_icons_loaded = true;
}
#endif

void  filetype_init(void)
{
    /* set the directory item first */
    filetypes[0].extension = NULL;
    filetypes[0].plugin = NULL;
    filetypes[0].attr   = 0;
    filetypes[0].icon   = Icon_Folder;
    
    filetype_count = 1;
    read_builtin_types();
    read_config(VIEWERS_CONFIG);
#ifdef HAVE_LCD_BITMAP
    read_viewer_theme_file();
#endif
#ifdef HAVE_LCD_COLOR
    read_color_theme_file();
#endif
}

/* remove all white spaces from string */
static void rm_whitespaces(char* str)
{
    char *s = str;
    while (*str)
    {
        if (!isspace(*str))
        {
            *s = *str;
            s++;
        }
        str++;
    }
    *s = '\0';
}

static void read_builtin_types(void)
{
    int count = sizeof(inbuilt_filetypes)/sizeof(*inbuilt_filetypes), i;
    for(i=0; i<count && (filetype_count < MAX_FILETYPES); i++)
    {
        filetypes[filetype_count].extension = inbuilt_filetypes[i].extension;
        filetypes[filetype_count].plugin = NULL;
        filetypes[filetype_count].attr   = inbuilt_filetypes[i].tree_attr>>8;
        if (filetypes[filetype_count].attr > highest_attr)
            highest_attr = filetypes[filetype_count].attr;
        filetypes[filetype_count].icon   = inbuilt_filetypes[i].icon;
        filetype_count++;
    }
}

static void read_config(char* config_file)
{
    char line[64], *s, *e;
    char extension[8], plugin[32];
    int fd = open(config_file, O_RDONLY);
    if (fd < 0)
        return;
    /* config file is in the for 
       <extension>,<plugin>,<icon code>
       ignore line if either of the first two are missing */
    while (read_line(fd, line, 64) > 0)
    {
        if (filetype_count >= MAX_FILETYPES)
        {
            splash(HZ, ID2P(LANG_FILETYPES_FULL));
            break;
        }
        rm_whitespaces(line);
        /* get the extention */
        s = line;
        e = strchr(s, ',');
        if (!e)
            continue;
        *e = '\0';
        strcpy(extension, s);
    
        /* get the plugin */
        s = e+1;
        e = strchr(s, ',');
        if (!e)
            continue;
        *e = '\0';
        
        strcpy(plugin, s);
        /* ok, store this plugin/extension, check icon after */
        filetypes[filetype_count].extension = filetypes_strdup(extension);
        filetypes[filetype_count].plugin = filetypes_strdup(plugin);
        filetypes[filetype_count].attr = highest_attr +1;
        filetypes[filetype_count].icon = Icon_Questionmark;
        highest_attr++;
        /* get the icon */
        s = e+1;
        if (*s == '*')
            filetypes[filetype_count].icon = atoi(s+1);
        else if (*s == '-')
            filetypes[filetype_count].icon = Icon_NOICON;
        else if (*s >= '0' && *s <= '9')
            filetypes[filetype_count].icon = Icon_Last_Themeable + atoi(s);
        filetype_count++;
    }
}

int filetype_get_attr(const char* file)
{
    char *extension = strrchr(file, '.');
    int i;
    if (!extension)
        return 0;
    extension++;
    for (i=0; i<filetype_count; i++)
    {
        if (filetypes[i].extension && 
            !strcasecmp(extension, filetypes[i].extension))
            return (filetypes[i].attr<<8)&FILE_ATTR_MASK;
    }
    return 0;
}

static int find_attr(int attr)
{
    int i;
    /* skip the directory item */
    if ((attr & ATTR_DIRECTORY)==ATTR_DIRECTORY)
        return 0;
    for (i=1; i<filetype_count; i++)
    {
        if ((attr>>8) == filetypes[i].attr)
            return i;
    }
    return -1;
}

#ifdef HAVE_LCD_COLOR
int filetype_get_color(const char * name, int attr)
{
    char *extension;
    int i;
    if ((attr & ATTR_DIRECTORY)==ATTR_DIRECTORY)
        return custom_colors[0];
    extension = strrchr(name, '.');
    if (!extension)
        return custom_colors[MAX_FILETYPES];
    extension++;
    
    for (i=1; i<filetype_count; i++)
    {
        if (filetypes[i].extension && 
            !strcasecmp(extension, filetypes[i].extension))
            return custom_colors[i];
    }
    return custom_colors[MAX_FILETYPES];
}
#endif

int filetype_get_icon(int attr)
{
    int index = find_attr(attr);
    if (index < 0)
        return Icon_NOICON;
    if (custom_icons_loaded)
        return custom_filetype_icons[index];
    return filetypes[index].icon;
}

char* filetype_get_plugin(const struct entry* file)
{
    static char plugin_name[MAX_PATH];
    int index = find_attr(file->attr);
    if (index < 0)
        return NULL;
    if (filetypes[index].plugin == NULL)
        return NULL;
    snprintf(plugin_name, MAX_PATH, "%s/%s.%s", 
             PLUGIN_DIR, filetypes[index].plugin, ROCK_EXTENSION);
    return plugin_name;
}

bool  filetype_supported(int attr)
{
    return find_attr(attr) >= 0;
}

/**** Open With Screen ****/
struct cb_data {
    int *items;
    const char *current_file;
};

static enum themable_icons openwith_get_icon(int selected_item, void * data)
{
    struct cb_data *info = (struct cb_data *)data;
    int *items = info->items;
    return filetypes[items[selected_item]].icon;
}

static const char* openwith_get_name(int selected_item, void * data,
                                     char * buffer, size_t buffer_len)
{
    (void)buffer; (void)buffer_len;
    struct cb_data *info = (struct cb_data *)data;
    int *items = info->items;
    const char *s = strrchr(filetypes[items[selected_item]].plugin, '/');
    if (s)
        return s+1;
    else return filetypes[items[selected_item]].plugin;
}

static int openwith_action_callback(int action, struct gui_synclist *lists)
{
    struct cb_data *info = (struct cb_data *)lists->data;
    int *items = info->items;
    int i;
    if (action == ACTION_STD_OK)
    {
        char plugin[MAX_PATH];
        i = items[gui_synclist_get_sel_pos(lists)];
        snprintf(plugin, MAX_PATH, "%s/%s.%s",
                    PLUGIN_DIR, filetypes[i].plugin, ROCK_EXTENSION);
        plugin_load(plugin, info->current_file);
        return ACTION_STD_CANCEL;
    }
    return action;
}

int filetype_list_viewers(const char* current_file)
{
    int i, count = 0;
    int items[MAX_FILETYPES];
    struct simplelist_info info;
    struct cb_data data = { items, current_file };
    for (i=0; i<filetype_count && count < MAX_FILETYPES; i++)
    {
        if (filetypes[i].plugin)
        {
            int j;
            for (j=0;j<count;j++) /* check if the plugin is in the list yet */
            {
                if (!strcmp(filetypes[i].plugin,filetypes[items[j]].plugin))
                    break;
            }
            if (j<count) 
                continue; /* it is so grab the next plugin */
            items[count++] = i;
        }
    }
#ifndef HAVE_LCD_BITMAP
    if (count == 0)
    {
        /* FIX: translation! */
        splash(HZ*2, "No viewers found");
        return PLUGIN_OK;
    }
#endif
    simplelist_info_init(&info, str(LANG_ONPLAY_OPEN_WITH), count, &data);
    info.action_callback = openwith_action_callback;
    info.get_name = openwith_get_name;
    info.get_icon = global_settings.show_icons?openwith_get_icon:NULL;
    return simplelist_show_list(&info);
}

int filetype_load_plugin(const char* plugin, char* file)
{
    int i;
    char plugin_name[MAX_PATH];
    char *s;
    
    for (i=0;i<filetype_count;i++)
    {
        if (filetypes[i].plugin)
        {
            s = strrchr(filetypes[i].plugin, '/');
            if (s)
            {
                if (!strcmp(s+1, plugin))
                    break;
            }
            else if (!strcmp(filetypes[i].plugin, plugin))
                break;
        }
    }
    if (i >= filetype_count)
        return PLUGIN_ERROR;
    snprintf(plugin_name, MAX_PATH, "%s/%s.%s",
             PLUGIN_DIR, filetypes[i].plugin, ROCK_EXTENSION);
    return plugin_load(plugin_name, file);
}
