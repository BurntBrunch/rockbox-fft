/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
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

#ifndef _SCREEN_ACCESS_H_
#define _SCREEN_ACCESS_H_

#include "lcd.h"
#include "buttonbar.h"
#include "backdrop.h"

#if defined(HAVE_REMOTE_LCD) && !defined (ROCKBOX_HAS_LOGF)
#define NB_SCREENS 2
#else
#define NB_SCREENS 1
#endif

#if NB_SCREENS == 1
#define FOR_NB_SCREENS(i) i = 0;
#else
#define FOR_NB_SCREENS(i) for(i = 0; i < NB_SCREENS; i++)
#endif

#ifdef HAVE_LCD_CHARCELLS
#define MAX_LINES_ON_SCREEN 2
#endif

typedef void screen_bitmap_part_func(const void *src, int src_x, int src_y,
                              int stride, int x, int y, int width, int height);
typedef void screen_bitmap_func(const void *src, int x, int y, int width,
                              int height);

/* if this struct is changed the plugin api may break so bump the api
   versions in plugin.h */
struct screen
{
    enum screen_type screen_type;
    int lcdwidth, lcdheight;
    int depth;
    int (*getnblines)(void);
#ifdef HAVE_LCD_BITMAP
    int pixel_format;
#endif
    int (*getcharwidth)(void);
    int (*getcharheight)(void);
    bool is_color;
#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
    bool has_disk_led;
#endif
#ifdef HAVE_BUTTONBAR
    bool has_buttonbar;
#endif
    void (*set_viewport)(struct viewport* vp);
    int (*getwidth)(void);
    int (*getheight)(void);
    int (*getstringsize)(const unsigned char *str, int *w, int *h);
#if defined(HAVE_LCD_BITMAP) || defined(HAVE_REMOTE_LCD) /* always bitmap */
    void (*setfont)(int newfont);
    int (*getfont)(void);

    void (*scroll_step)(int pixels);
    void (*puts_style_offset)(int x, int y, const unsigned char *str,
                              int style, int offset);
    void (*puts_scroll_style)(int x, int y, const unsigned char *string,
                                 int style);
    void (*puts_scroll_style_offset)(int x, int y, const unsigned char *string,
                                     int style, int offset);
    void (*mono_bitmap)(const unsigned char *src,
                        int x, int y, int width, int height);
    void (*mono_bitmap_part)(const unsigned char *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height);
    void (*bitmap)(const void *src,
                   int x, int y, int width, int height);
    void (*bitmap_part)(const void *src, int src_x, int src_y,
                          int stride, int x, int y, int width, int height);
    void (*transparent_bitmap)(const void *src,
                               int x, int y, int width, int height);
    void (*transparent_bitmap_part)(const void *src, int src_x, int src_y,
                                    int stride, int x, int y, int width, int height);
    void (*set_drawmode)(int mode);
#if defined(HAVE_LCD_COLOR) && defined(LCD_REMOTE_DEPTH) && LCD_REMOTE_DEPTH > 1
    unsigned (*color_to_native)(unsigned color);
#endif
#if (LCD_DEPTH > 1) || (defined(LCD_REMOTE_DEPTH) && (LCD_REMOTE_DEPTH > 1))
    unsigned (*get_background)(void);
    unsigned (*get_foreground)(void);
    void (*set_background)(unsigned background);
    void (*set_foreground)(unsigned foreground);
#endif /* (LCD_DEPTH > 1) || (LCD_REMOTE_DEPTH > 1) */
#if defined(HAVE_LCD_COLOR)
    void (*set_selector_start)(unsigned selector);
    void (*set_selector_end)(unsigned selector);
    void (*set_selector_text)(unsigned selector_text);
#endif
    void (*update_rect)(int x, int y, int width, int height);
    void (*update_viewport_rect)(int x, int y, int width, int height);
    void (*fillrect)(int x, int y, int width, int height);
    void (*drawrect)(int x, int y, int width, int height);
    void (*drawpixel)(int x, int y);
    void (*drawline)(int x1, int y1, int x2, int y2);
    void (*vline)(int x, int y1, int y2);
    void (*hline)(int x1, int x2, int y);
#endif /* HAVE_LCD_BITMAP || HAVE_REMOTE_LCD */

#ifdef HAVE_LCD_CHARCELLS  /* no charcell remote LCDs so far */
    void (*double_height)(bool on);
    void (*putc)(int x, int y, unsigned long ucs);
    void (*icon)(int icon, bool enable);
    unsigned long (*get_locked_pattern)(void);
    void (*define_pattern)(unsigned long ucs, const char *pattern);
    void (*unlock_pattern)(unsigned long ucs);
#endif
    void (*putsxy)(int x, int y, const unsigned char *str);
    void (*puts)(int x, int y, const unsigned char *str);
    void (*putsf)(int x, int y, const unsigned char *str, ...);
    void (*puts_offset)(int x, int y, const unsigned char *str, int offset);
    void (*puts_scroll)(int x, int y, const unsigned char *string);
    void (*puts_scroll_offset)(int x, int y, const unsigned char *string,
                                 int offset);
    void (*scroll_speed)(int speed);
    void (*scroll_delay)(int ms);
    void (*stop_scroll)(void);
    void (*clear_display)(void);
    void (*clear_viewport)(void);
    void (*scroll_stop)(const struct viewport* vp);
    void (*scroll_stop_line)(const struct viewport* vp, int y);
    void (*update)(void);
    void (*update_viewport)(void);
    void (*backlight_on)(void);
    void (*backlight_off)(void);
    bool (*is_backlight_on)(bool ignore_always_off);
    void (*backlight_set_timeout)(int index);
#if LCD_DEPTH > 1
    bool (*backdrop_load)(enum backdrop_type bdrop, const char* filename);
    void (*backdrop_unload)(enum backdrop_type bdrop);
    void (*backdrop_show)(enum backdrop_type bdrop);
    void (*backdrop_hide)(void);
#endif
};

#if defined(HAVE_LCD_BITMAP) || defined(HAVE_REMOTE_LCD)
/*
 * Clear only a given area of the screen
 * - screen : the screen structure
 * - xstart, ystart : where the area starts
 * - width, height : size of the area
 */
void screen_clear_area(struct screen * display, int xstart, int ystart,
                       int width, int height);
#endif

/*
 * exported screens array that should be used
 * by each app that wants to write to access display
 */
extern struct screen screens[NB_SCREENS];

#endif /*_SCREEN_ACCESS_H_*/
