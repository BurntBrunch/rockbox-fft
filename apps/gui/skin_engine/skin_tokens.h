/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
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
 
#ifndef _SKIN_TOKENS_H_
#define _SKIN_TOKENS_H_

#include <stdbool.h>
 

enum wps_token_type {
    
  TOKEN_MARKER_CONTROL_TOKENS = -1,
    WPS_NO_TOKEN = 0,   /* for WPS tags we don't want to save as tokens */
    WPS_TOKEN_UNKNOWN,

    /* Markers */
    WPS_TOKEN_CHARACTER,
    WPS_TOKEN_STRING,
    WPS_TOKEN_TRANSLATEDSTRING,

    /* Alignment */
    WPS_TOKEN_ALIGN_LEFT,
    WPS_TOKEN_ALIGN_LEFT_RTL,
    WPS_TOKEN_ALIGN_CENTER,
    WPS_TOKEN_ALIGN_RIGHT,
    WPS_TOKEN_ALIGN_RIGHT_RTL,

    /* Sublines */
    WPS_TOKEN_SUBLINE_TIMEOUT,
    
    /* Conditional */
    WPS_TOKEN_CONDITIONAL,
    WPS_TOKEN_CONDITIONAL_START,
    WPS_TOKEN_CONDITIONAL_OPTION,
    WPS_TOKEN_CONDITIONAL_END,

    /* Viewport display */
    WPS_VIEWPORT_ENABLE,
    WPS_VIEWPORT_CUSTOMLIST,
    
    /* Battery */
  TOKEN_MARKER_BATTERY,
    WPS_TOKEN_BATTERY_PERCENT,
    WPS_TOKEN_BATTERY_VOLTS,
    WPS_TOKEN_BATTERY_TIME,
    WPS_TOKEN_BATTERY_CHARGER_CONNECTED,
    WPS_TOKEN_BATTERY_CHARGING,
    WPS_TOKEN_BATTERY_SLEEPTIME,
    WPS_TOKEN_USB_POWERED,

    /* Sound */
  TOKEN_MARKER_SOUND,
#if (CONFIG_CODEC != MAS3507D)
    WPS_TOKEN_SOUND_PITCH,
#endif
#if (CONFIG_CODEC == SWCODEC)
    WPS_TOKEN_SOUND_SPEED,
    WPS_TOKEN_REPLAYGAIN,
    WPS_TOKEN_CROSSFADE,
#endif

    /* Time */
  TOKEN_MARKER_RTC,
    WPS_TOKEN_RTC_PRESENT,

    /* The begin/end values allow us to know if a token is an RTC one.
       New RTC tokens should be added between the markers. */

    WPS_TOKENS_RTC_BEGIN, /* just the start marker, not an actual token */

    WPS_TOKEN_RTC_DAY_OF_MONTH,
    WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED,
    WPS_TOKEN_RTC_12HOUR_CFG,
    WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED,
    WPS_TOKEN_RTC_HOUR_24,
    WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED,
    WPS_TOKEN_RTC_HOUR_12,
    WPS_TOKEN_RTC_MONTH,
    WPS_TOKEN_RTC_MINUTE,
    WPS_TOKEN_RTC_SECOND,
    WPS_TOKEN_RTC_YEAR_2_DIGITS,
    WPS_TOKEN_RTC_YEAR_4_DIGITS,
    WPS_TOKEN_RTC_AM_PM_UPPER,
    WPS_TOKEN_RTC_AM_PM_LOWER,
    WPS_TOKEN_RTC_WEEKDAY_NAME,
    WPS_TOKEN_RTC_MONTH_NAME,
    WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON,
    WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN,

    WPS_TOKENS_RTC_END,     /* just the end marker, not an actual token */

    /* Database */
  TOKEN_MARKER_DATABASE,
#ifdef HAVE_TAGCACHE
    WPS_TOKEN_DATABASE_PLAYCOUNT,
    WPS_TOKEN_DATABASE_RATING,
    WPS_TOKEN_DATABASE_AUTOSCORE,
#endif

    /* File */
  TOKEN_MARKER_FILE,
    WPS_TOKEN_FILE_BITRATE,
    WPS_TOKEN_FILE_CODEC,
    WPS_TOKEN_FILE_FREQUENCY,
    WPS_TOKEN_FILE_FREQUENCY_KHZ,
    WPS_TOKEN_FILE_NAME,
    WPS_TOKEN_FILE_NAME_WITH_EXTENSION,
    WPS_TOKEN_FILE_PATH,
    WPS_TOKEN_FILE_SIZE,
    WPS_TOKEN_FILE_VBR,
    WPS_TOKEN_FILE_DIRECTORY,

    /* Image */
  TOKEN_MARKER_IMAGES,
#ifdef HAVE_LCD_BITMAP
    WPS_TOKEN_IMAGE_BACKDROP,
    WPS_TOKEN_IMAGE_PROGRESS_BAR,
    WPS_TOKEN_IMAGE_PRELOAD,
    WPS_TOKEN_IMAGE_PRELOAD_DISPLAY,
    WPS_TOKEN_IMAGE_DISPLAY,
#endif

#ifdef HAVE_ALBUMART
    /* Albumart */
    WPS_TOKEN_ALBUMART_DISPLAY,
    WPS_TOKEN_ALBUMART_FOUND,
#endif

    /* Metadata */
  TOKEN_MARKER_METADATA,
    WPS_TOKEN_METADATA_ARTIST,
    WPS_TOKEN_METADATA_COMPOSER,
    WPS_TOKEN_METADATA_ALBUM_ARTIST,
    WPS_TOKEN_METADATA_GROUPING,
    WPS_TOKEN_METADATA_ALBUM,
    WPS_TOKEN_METADATA_GENRE,
    WPS_TOKEN_METADATA_DISC_NUMBER,
    WPS_TOKEN_METADATA_TRACK_NUMBER,
    WPS_TOKEN_METADATA_TRACK_TITLE,
    WPS_TOKEN_METADATA_VERSION,
    WPS_TOKEN_METADATA_YEAR,
    WPS_TOKEN_METADATA_COMMENT,

  TOKEN_MARKER_PLAYBACK_INFO,
    /* Mode */
    WPS_TOKEN_REPEAT_MODE,
    WPS_TOKEN_PLAYBACK_STATUS,
    /* Progressbar */
    WPS_TOKEN_PROGRESSBAR,
#ifdef HAVE_LCD_CHARCELLS
    WPS_TOKEN_PLAYER_PROGRESSBAR,
#endif
#ifdef HAVE_LCD_BITMAP
    /* Peakmeter */
    WPS_TOKEN_PEAKMETER,
#endif

    /* Current track */
    WPS_TOKEN_TRACK_ELAPSED_PERCENT,
    WPS_TOKEN_TRACK_TIME_ELAPSED,
    WPS_TOKEN_TRACK_TIME_REMAINING,
    WPS_TOKEN_TRACK_LENGTH,

    /* Playlist */
  TOKEN_MARKER_PLAYLIST,
    WPS_TOKEN_PLAYLIST_ENTRIES,
    WPS_TOKEN_PLAYLIST_NAME,
    WPS_TOKEN_PLAYLIST_POSITION,
    WPS_TOKEN_PLAYLIST_SHUFFLE,


    /* buttons */
  TOKEN_MARKER_MISC,
    WPS_TOKEN_BUTTON_VOLUME,
    WPS_TOKEN_LASTTOUCH,
#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
    /* Virtual LED */
    WPS_TOKEN_VLED_HDD,
#endif
    /* Volume level */
    WPS_TOKEN_VOLUME,
    /* hold */
    WPS_TOKEN_MAIN_HOLD,
#ifdef HAS_REMOTE_BUTTON_HOLD
    WPS_TOKEN_REMOTE_HOLD,
#endif

    /* Setting option */
    WPS_TOKEN_SETTING,
    WPS_TOKEN_CURRENT_SCREEN,
    WPS_TOKEN_LANG_IS_RTL,
    
    /* Recording Tokens */
  TOKEN_MARKER_RECORDING,
    WPS_TOKEN_HAVE_RECORDING,
    WPS_TOKEN_REC_FREQ,
    WPS_TOKEN_REC_ENCODER,
    WPS_TOKEN_REC_BITRATE, /* SWCODEC: MP3 bitrate, HWCODEC: MP3 "quality" */
    WPS_TOKEN_REC_MONO,
    
  TOKEN_MARKER_END, /* this needs to be the last value in this enum */
};

struct wps_token {
    unsigned char type; /* enough to store the token type */

    /* Whether the tag (e.g. track name or the album) refers the
       current or the next song (false=current, true=next) */
    bool next;

    union {
        char c;
        unsigned short i;
        void* data;
    } value;
};
 
struct skin_token_list {
    struct wps_token *token;
    struct skin_token_list *next;
};

char* get_dir(char* buf, int buf_size, const char* path, int level);
 
#endif
 
