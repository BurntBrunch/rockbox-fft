/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
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

/* Button Code Definitions for touchscreen targets */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "action.h"
#include "button.h"
#include "settings.h"

const struct button_mapping* target_get_context_mapping(int context);
/* How this file is used:
   get_context_mapping() at the bottom of the file is called by action.c as usual.
   if the context is for the remote control its then passed straight to
   target_get_context_mapping().
   These tables are only used for the touchscreen buttons, so at the end of each
   CONTEXT_CUSTOM2 is OR'ed with the context and then sent to target_get_context_mapping()
   (NOTE: CONTEXT_CUSTOM2 will be stripped before being sent to make it easier.)
   In the target keymap, remember to |CONTEXT_CUSTOM2 in the  LAST_ITEM_IN_LIST__NEXTLIST() macro
   to speed it up a tiny bit... if you dont it will go through these tables first before going
   back to the target file.
 */


/* touchscreen "buttons"
   screen is split into a 3x3 grid for buttons...
    BUTTON_TOPLEFT      BUTTON_TOPMIDDLE    BUTTON_TOPRIGHT
    BUTTON_MIDLEFT      BUTTON_CENTER       BUTTON_MIDRIGHT
    BUTTON_BOTTOMLEFT   BUTTON_BOTTOMMIDDLE BUTTON_BOTTOMRIGHT
*/

static const struct button_mapping button_context_standard[]  = {
    { ACTION_STD_PREV,        BUTTON_TOPMIDDLE,                  BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,  BUTTON_TOPMIDDLE|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_STD_NEXT,        BUTTON_BOTTOMMIDDLE,               BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,  BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT, BUTTON_NONE },

    { ACTION_STD_OK,          BUTTON_CENTER|BUTTON_REL,          BUTTON_CENTER },
    { ACTION_STD_OK,          BUTTON_MIDRIGHT|BUTTON_REL,        BUTTON_MIDRIGHT },
    { ACTION_STD_CANCEL,      BUTTON_MIDLEFT,                    BUTTON_NONE },
    { ACTION_STD_CANCEL,      BUTTON_MIDLEFT|BUTTON_REPEAT,      BUTTON_NONE },

    { ACTION_STD_MENU,        BUTTON_TOPLEFT,                    BUTTON_NONE },
    { ACTION_STD_QUICKSCREEN, BUTTON_TOPLEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_STD_CONTEXT,     BUTTON_CENTER|BUTTON_REPEAT,       BUTTON_CENTER },
    { ACTION_STD_CONTEXT,     BUTTON_MIDRIGHT|BUTTON_REPEAT,     BUTTON_MIDRIGHT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_STD)
}; /* button_context_standard */

static const struct button_mapping button_context_wps[]  = {

    { ACTION_WPS_PLAY,          BUTTON_TOPRIGHT|BUTTON_REL,         BUTTON_TOPRIGHT },
    { ACTION_WPS_STOP,          BUTTON_TOPRIGHT|BUTTON_REPEAT,      BUTTON_TOPRIGHT },

    { ACTION_WPS_SKIPPREV,      BUTTON_MIDLEFT|BUTTON_REL,          BUTTON_MIDLEFT },
    { ACTION_WPS_SEEKBACK,      BUTTON_MIDLEFT|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,      BUTTON_MIDLEFT|BUTTON_REL,          BUTTON_MIDLEFT|BUTTON_REPEAT },

    { ACTION_WPS_SKIPNEXT,      BUTTON_MIDRIGHT|BUTTON_REL,         BUTTON_MIDRIGHT },
    { ACTION_WPS_SEEKFWD,       BUTTON_MIDRIGHT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_WPS_STOPSEEK,      BUTTON_MIDRIGHT|BUTTON_REL,         BUTTON_MIDRIGHT|BUTTON_REPEAT },

    { ACTION_WPS_VOLUP,         BUTTON_TOPMIDDLE,                   BUTTON_NONE },
    { ACTION_WPS_VOLUP,         BUTTON_TOPMIDDLE|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_BOTTOMMIDDLE,                BUTTON_NONE },
    { ACTION_WPS_VOLDOWN,       BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT,  BUTTON_NONE },

    { ACTION_WPS_BROWSE,        BUTTON_CENTER|BUTTON_REL,           BUTTON_CENTER },
    { ACTION_WPS_CONTEXT,       BUTTON_CENTER|BUTTON_REPEAT,        BUTTON_CENTER },

    { ACTION_WPS_MENU,          BUTTON_TOPLEFT|BUTTON_REL,          BUTTON_TOPLEFT },
    { ACTION_WPS_QUICKSCREEN,   BUTTON_TOPLEFT|BUTTON_REPEAT,       BUTTON_TOPLEFT },
    { ACTION_WPS_VIEW_PLAYLIST, BUTTON_BOTTOMLEFT|BUTTON_REL,       BUTTON_BOTTOMLEFT },
    { ACTION_WPS_REC,           BUTTON_BOTTOMRIGHT|BUTTON_REPEAT,   BUTTON_BOTTOMRIGHT },
    { ACTION_WPS_PITCHSCREEN,   BUTTON_BOTTOMLEFT|BUTTON_REPEAT,    BUTTON_BOTTOMLEFT },
    { ACTION_WPS_ID3SCREEN,     BUTTON_BOTTOMRIGHT|BUTTON_REL,      BUTTON_BOTTOMRIGHT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_WPS)
}; /* button_context_wps */

static const struct button_mapping button_context_list[]  = {
#if 0
    /* this is all to show how the poor-mans-gestures can be used... */
    { ACTION_LISTTREE_PGUP,     BUTTON_TOPRIGHT,                BUTTON_NONE},
    { ACTION_LISTTREE_PGUP,     BUTTON_TOPRIGHT|BUTTON_REPEAT,  BUTTON_TOPRIGHT},
    { ACTION_STD_NEXTREPEAT,    BUTTON_CENTER,                  BUTTON_TOPMIDDLE},
    { ACTION_STD_NEXTREPEAT,    BUTTON_BOTTOMMIDDLE,            BUTTON_CENTER},
    { ACTION_NONE,              BUTTON_TOPMIDDLE,               BUTTON_NONE },
    { ACTION_NONE,              BUTTON_BOTTOMMIDDLE,            BUTTON_NONE },
    { ACTION_STD_PREV,          BUTTON_TOPMIDDLE|BUTTON_REL,    BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_BOTTOMMIDDLE|BUTTON_REL, BUTTON_NONE },
    { ACTION_LISTTREE_PGDOWN,   BUTTON_BOTTOMRIGHT,             BUTTON_NONE},
    { ACTION_LISTTREE_PGDOWN,   BUTTON_BOTTOMRIGHT|BUTTON_REPEAT, BUTTON_BOTTOMRIGHT},
    { ACTION_STD_PREVREPEAT,    BUTTON_TOPMIDDLE,               BUTTON_CENTER},
    { ACTION_STD_PREVREPEAT,    BUTTON_CENTER,                  BUTTON_BOTTOMMIDDLE},
#endif
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_LIST)
}; /* button_context_list */

static const struct button_mapping button_context_tree[]  = {
    { ACTION_TREE_WPS,    BUTTON_TOPRIGHT|BUTTON_REL,         BUTTON_TOPRIGHT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_CUSTOM|CONTEXT_TREE)
}; /* button_context_tree */

static const struct button_mapping button_context_listtree_scroll_with_combo[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_listtree_scroll_without_combo[]  = {
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_CUSTOM|CONTEXT_TREE),
};

static const struct button_mapping button_context_settings[]  = {
    { ACTION_SETTINGS_INC,          BUTTON_TOPMIDDLE,                   BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_TOPMIDDLE|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_BOTTOMMIDDLE,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_SETTINGS_RESET,        BUTTON_CENTER,                      BUTTON_NONE },

    { ACTION_STD_PREV,          BUTTON_MIDLEFT,                         BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,    BUTTON_MIDLEFT|BUTTON_REPEAT,           BUTTON_NONE },
    { ACTION_STD_NEXT,          BUTTON_MIDRIGHT,                        BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,    BUTTON_MIDRIGHT|BUTTON_REPEAT,          BUTTON_NONE },
    { ACTION_STD_CANCEL,        BUTTON_TOPLEFT,                         BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_SETTINGS)
}; /* button_context_settings */

static const struct button_mapping button_context_settings_right_is_inc[]  = {
    { ACTION_NONE,                  BUTTON_MIDRIGHT|BUTTON_REL,     BUTTON_MIDRIGHT },
    { ACTION_NONE,                  BUTTON_MIDRIGHT|BUTTON_REPEAT,  BUTTON_MIDRIGHT },
    { ACTION_SETTINGS_INC,          BUTTON_MIDRIGHT,                BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT,    BUTTON_MIDRIGHT|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_SETTINGS_DEC,          BUTTON_MIDLEFT,                 BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT,    BUTTON_MIDLEFT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_SETTINGS_RESET,        BUTTON_CENTER,                  BUTTON_NONE },

    { ACTION_STD_CANCEL,            BUTTON_TOPLEFT,                 BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_CUSTOM|CONTEXT_SETTINGS)
}; /* button_context_settingsgraphical */

static const struct button_mapping button_context_yesno[]  = {
    { ACTION_YESNO_ACCEPT,  BUTTON_TOPRIGHT,  BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_YESNOSCREEN)
}; /* button_context_settings_yesno */

static const struct button_mapping button_context_colorchooser[]  = {
    { ACTION_STD_OK,  BUTTON_CENTER|BUTTON_REL,  BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_SETTINGS_COLOURCHOOSER),
}; /* button_context_colorchooser */

static const struct button_mapping button_context_eq[]  = {
    /* override the std context since we're using MIDRIGHT for settings inc */
    { ACTION_NONE,          BUTTON_MIDRIGHT|BUTTON_REL,        BUTTON_MIDRIGHT },
    { ACTION_NONE,          BUTTON_MIDRIGHT|BUTTON_REPEAT,     BUTTON_MIDRIGHT },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_SETTINGS_EQ),
}; /* button_context_eq */

/* Bookmark Screen */
static const struct button_mapping button_context_bmark[]  = {
    { ACTION_BMS_DELETE,    BUTTON_BOTTOMLEFT,                 BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_BOOKMARKSCREEN),
}; /* button_context_bmark */

static const struct button_mapping button_context_time[]  = {
    { ACTION_SETTINGS_INC,       BUTTON_TOPMIDDLE,                   BUTTON_NONE },
    { ACTION_SETTINGS_INCREPEAT, BUTTON_TOPMIDDLE|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_SETTINGS_DEC,       BUTTON_BOTTOMMIDDLE,                BUTTON_NONE },
    { ACTION_SETTINGS_DECREPEAT, BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT,  BUTTON_NONE },
    { ACTION_STD_PREV,           BUTTON_MIDLEFT,                     BUTTON_NONE },
    { ACTION_STD_PREVREPEAT,     BUTTON_MIDLEFT|BUTTON_REPEAT,       BUTTON_NONE },
    { ACTION_STD_NEXT,           BUTTON_MIDRIGHT,                    BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT,     BUTTON_MIDRIGHT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_STD_OK,             BUTTON_CENTER|BUTTON_REL,           BUTTON_CENTER  },
    { ACTION_STD_CANCEL,         BUTTON_TOPLEFT|BUTTON_REL,          BUTTON_TOPLEFT },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_SETTINGS_TIME),
}; /* button_context_time */

static const struct button_mapping button_context_quickscreen[]  = {
    /* override ACTION_STD_CANCEL from standard context */
    { ACTION_NONE,       BUTTON_MIDLEFT,                    BUTTON_NONE },

    { ACTION_STD_CANCEL, BUTTON_CENTER|BUTTON_REL,          BUTTON_NONE },
    { ACTION_QS_TOP,     BUTTON_TOPMIDDLE|BUTTON_REL,       BUTTON_NONE },
    { ACTION_QS_TOP,     BUTTON_TOPMIDDLE|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_QS_DOWN,    BUTTON_BOTTOMMIDDLE|BUTTON_REL,    BUTTON_NONE },
    { ACTION_QS_DOWN,    BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_QS_LEFT,    BUTTON_MIDLEFT|BUTTON_REL,         BUTTON_NONE },
    { ACTION_QS_LEFT,    BUTTON_MIDLEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_QS_RIGHT,   BUTTON_MIDRIGHT|BUTTON_REL,        BUTTON_NONE },
    { ACTION_QS_RIGHT,   BUTTON_MIDRIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_QUICKSCREEN)
}; /* button_context_quickscreen */

static const struct button_mapping button_context_pitchscreen[]  = {

    { ACTION_PS_INC_SMALL,      BUTTON_TOPMIDDLE,                  BUTTON_NONE },
    { ACTION_PS_INC_BIG,        BUTTON_TOPMIDDLE|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_PS_DEC_SMALL,      BUTTON_BOTTOMMIDDLE,               BUTTON_NONE },
    { ACTION_PS_DEC_BIG,        BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFT,     BUTTON_MIDLEFT,                    BUTTON_NONE },
    { ACTION_PS_NUDGE_LEFTOFF,  BUTTON_MIDLEFT|BUTTON_REL,         BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHT,    BUTTON_MIDRIGHT,                   BUTTON_NONE },
    { ACTION_PS_NUDGE_RIGHTOFF, BUTTON_MIDRIGHT|BUTTON_REL,        BUTTON_NONE },
    { ACTION_PS_TOGGLE_MODE,    BUTTON_BOTTOMRIGHT,                BUTTON_NONE },
    { ACTION_PS_RESET,          BUTTON_CENTER,                     BUTTON_NONE },
    { ACTION_PS_EXIT,           BUTTON_TOPLEFT,                    BUTTON_NONE },
    { ACTION_PS_SLOWER,         BUTTON_MIDLEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_PS_FASTER,         BUTTON_MIDRIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_PITCHSCREEN)
}; /* button_context_pitchcreen */

static const struct button_mapping button_context_keyboard[]  = {

    { ACTION_KBD_LEFT,         BUTTON_MIDLEFT,                    BUTTON_NONE },
    { ACTION_KBD_LEFT,         BUTTON_MIDLEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_MIDRIGHT,                   BUTTON_NONE },
    { ACTION_KBD_RIGHT,        BUTTON_MIDRIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_TOPLEFT,                    BUTTON_NONE },
    { ACTION_KBD_CURSOR_LEFT,  BUTTON_TOPLEFT|BUTTON_REPEAT,      BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_TOPRIGHT,                   BUTTON_NONE },
    { ACTION_KBD_CURSOR_RIGHT, BUTTON_TOPRIGHT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_KBD_SELECT,       BUTTON_CENTER|BUTTON_REL,          BUTTON_NONE },
    { ACTION_KBD_DONE,         BUTTON_CENTER|BUTTON_REPEAT,       BUTTON_CENTER },
    { ACTION_KBD_ABORT,        BUTTON_BOTTOMRIGHT,                BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_BOTTOMLEFT,                 BUTTON_NONE },
    { ACTION_KBD_BACKSPACE,    BUTTON_BOTTOMLEFT|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_TOPMIDDLE,                  BUTTON_NONE },
    { ACTION_KBD_UP,           BUTTON_TOPMIDDLE|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_BOTTOMMIDDLE,               BUTTON_NONE },
    { ACTION_KBD_DOWN,         BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_KEYBOARD)
}; /* button_context_keyboard */

static const struct button_mapping button_context_radio[]  = {
    { ACTION_STD_PREV,       BUTTON_MIDLEFT,                   BUTTON_NONE },
    { ACTION_STD_PREVREPEAT, BUTTON_MIDLEFT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_STD_NEXT,       BUTTON_MIDRIGHT,                  BUTTON_NONE },
    { ACTION_STD_NEXTREPEAT, BUTTON_MIDRIGHT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_FM_MENU,        BUTTON_CENTER|BUTTON_REPEAT,      BUTTON_CENTER },
    { ACTION_FM_PRESET,      BUTTON_CENTER|BUTTON_REL,         BUTTON_NONE },
    { ACTION_FM_STOP,        BUTTON_BOTTOMRIGHT|BUTTON_REL,    BUTTON_NONE },
    { ACTION_FM_EXIT,        BUTTON_TOPLEFT|BUTTON_REL,        BUTTON_NONE },
    { ACTION_FM_MODE,        BUTTON_BOTTOMLEFT|BUTTON_REL,     BUTTON_NONE },
    { ACTION_FM_PLAY,        BUTTON_TOPRIGHT|BUTTON_REL,       BUTTON_NONE },
    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_CUSTOM2|CONTEXT_FM)
}; /* button_context_radio */

#ifdef USB_ENABLE_HID
static const struct button_mapping button_context_usb_hid[] = {
    { ACTION_USB_HID_MODE_SWITCH_NEXT, BUTTON_TOPLEFT|BUTTON_REL,    BUTTON_TOPLEFT },
    { ACTION_USB_HID_MODE_SWITCH_PREV, BUTTON_TOPLEFT|BUTTON_REPEAT, BUTTON_TOPLEFT },

    LAST_ITEM_IN_LIST
}; /* button_context_usb_hid */

static const struct button_mapping button_context_usb_hid_mode_multimedia[] = {
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_BOTTOMMIDDLE,               BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_DOWN,         BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT, BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_TOPMIDDLE,                  BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_UP,           BUTTON_TOPMIDDLE|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_USB_HID_MULTIMEDIA_VOLUME_MUTE,         BUTTON_CENTER|BUTTON_REL,          BUTTON_CENTER },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_PLAY_PAUSE, BUTTON_TOPRIGHT|BUTTON_REL,        BUTTON_TOPRIGHT },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_STOP,       BUTTON_TOPRIGHT|BUTTON_REPEAT,     BUTTON_TOPRIGHT },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_PREV, BUTTON_BOTTOMLEFT|BUTTON_REL,      BUTTON_BOTTOMLEFT },
    { ACTION_USB_HID_MULTIMEDIA_PLAYBACK_TRACK_NEXT, BUTTON_BOTTOMRIGHT|BUTTON_REL,     BUTTON_BOTTOMRIGHT },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_multimedia */

static const struct button_mapping button_context_usb_hid_mode_presentation[] = {
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_START, BUTTON_TOPRIGHT|BUTTON_REL,        BUTTON_TOPRIGHT },
    { ACTION_USB_HID_PRESENTATION_SLIDESHOW_LEAVE, BUTTON_TOPRIGHT|BUTTON_REPEAT,     BUTTON_TOPRIGHT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_PREV,      BUTTON_MIDLEFT|BUTTON_REL,         BUTTON_MIDLEFT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_NEXT,      BUTTON_MIDRIGHT|BUTTON_REL,        BUTTON_MIDRIGHT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_FIRST,     BUTTON_MIDLEFT|BUTTON_REPEAT,      BUTTON_MIDLEFT },
    { ACTION_USB_HID_PRESENTATION_SLIDE_LAST,      BUTTON_MIDRIGHT|BUTTON_REPEAT,     BUTTON_MIDRIGHT },
    { ACTION_USB_HID_PRESENTATION_SCREEN_BLACK,    BUTTON_BOTTOMRIGHT|BUTTON_REL,     BUTTON_BOTTOMRIGHT },
    { ACTION_USB_HID_PRESENTATION_SCREEN_WHITE,    BUTTON_BOTTOMRIGHT|BUTTON_REPEAT,  BUTTON_BOTTOMRIGHT },
    { ACTION_USB_HID_PRESENTATION_LINK_PREV,       BUTTON_TOPMIDDLE|BUTTON_REL,       BUTTON_TOPMIDDLE },
    { ACTION_USB_HID_PRESENTATION_LINK_PREV,       BUTTON_TOPMIDDLE|BUTTON_REPEAT,    BUTTON_TOPMIDDLE },
    { ACTION_USB_HID_PRESENTATION_LINK_NEXT,       BUTTON_BOTTOMMIDDLE|BUTTON_REL,    BUTTON_BOTTOMMIDDLE },
    { ACTION_USB_HID_PRESENTATION_LINK_NEXT,       BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT, BUTTON_BOTTOMMIDDLE },
    { ACTION_USB_HID_PRESENTATION_MOUSE_CLICK,     BUTTON_CENTER|BUTTON_REL,          BUTTON_CENTER },
    { ACTION_USB_HID_PRESENTATION_MOUSE_OVER,      BUTTON_CENTER|BUTTON_REPEAT,       BUTTON_CENTER },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_presentation */

static const struct button_mapping button_context_usb_hid_mode_browser[] = {
    { ACTION_USB_HID_BROWSER_SCROLL_UP,        BUTTON_TOPMIDDLE,                                 BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_UP,        BUTTON_TOPMIDDLE|BUTTON_REPEAT,                   BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,      BUTTON_BOTTOMMIDDLE,                              BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_SCROLL_DOWN,      BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT,                BUTTON_NONE },
    { ACTION_USB_HID_BROWSER_ZOOM_IN,          BUTTON_TOPRIGHT|BUTTON_REPEAT,                    BUTTON_TOPRIGHT },
    { ACTION_USB_HID_BROWSER_ZOOM_OUT,         BUTTON_BOTTOMRIGHT|BUTTON_REPEAT,                 BUTTON_BOTTOMRIGHT },
    { ACTION_USB_HID_BROWSER_ZOOM_RESET,       BUTTON_CENTER|BUTTON_REPEAT,                      BUTTON_CENTER },
    { ACTION_USB_HID_BROWSER_ZOOM_RESET,       BUTTON_TOPRIGHT|BUTTON_BOTTOMRIGHT|BUTTON_REPEAT, BUTTON_TOPRIGHT|BUTTON_BOTTOMRIGHT },
    { ACTION_USB_HID_BROWSER_TAB_PREV,         BUTTON_MIDLEFT|BUTTON_REL,                        BUTTON_MIDLEFT },
    { ACTION_USB_HID_BROWSER_TAB_NEXT,         BUTTON_MIDRIGHT|BUTTON_REL,                       BUTTON_MIDRIGHT },
    { ACTION_USB_HID_BROWSER_TAB_CLOSE,        BUTTON_BOTTOMLEFT|BUTTON_REPEAT,                  BUTTON_BOTTOMLEFT },
    { ACTION_USB_HID_BROWSER_HISTORY_BACK,     BUTTON_MIDLEFT|BUTTON_REPEAT,                     BUTTON_MIDLEFT },
    { ACTION_USB_HID_BROWSER_HISTORY_FORWARD,  BUTTON_MIDRIGHT|BUTTON_REPEAT,                    BUTTON_MIDRIGHT },
    { ACTION_USB_HID_BROWSER_VIEW_FULL_SCREEN, BUTTON_CENTER|BUTTON_REL,                         BUTTON_CENTER },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_browser */

#ifdef HAVE_USB_HID_MOUSE
static const struct button_mapping button_context_usb_hid_mode_mouse[] = {
    { ACTION_USB_HID_MOUSE_UP,                BUTTON_TOPMIDDLE,                 BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_UP_REP,            BUTTON_TOPMIDDLE|BUTTON_REPEAT,   BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_DOWN,              BUTTON_BOTTOMMIDDLE,              BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_DOWN_REP,          BUTTON_BOTTOMMIDDLE|BUTTON_REPEAT,BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LEFT,              BUTTON_MIDLEFT,                   BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_LEFT_REP,          BUTTON_MIDLEFT|BUTTON_REPEAT,     BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RIGHT,             BUTTON_MIDRIGHT,                  BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_RIGHT_REP,         BUTTON_MIDRIGHT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_LEFT,       BUTTON_CENTER,                    BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_BUTTON_LEFT_REL,   BUTTON_CENTER|BUTTON_REL,         BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   BUTTON_TOPRIGHT,                  BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_UP,   BUTTON_TOPRIGHT|BUTTON_REPEAT,    BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, BUTTON_BOTTOMRIGHT,               BUTTON_NONE },
    { ACTION_USB_HID_MOUSE_WHEEL_SCROLL_DOWN, BUTTON_BOTTOMRIGHT|BUTTON_REPEAT, BUTTON_NONE },

    LAST_ITEM_IN_LIST__NEXTLIST(CONTEXT_USB_HID)
}; /* button_context_usb_hid_mode_mouse */
#endif
#endif

const struct button_mapping* get_context_mapping(int context)
{
    if (context & CONTEXT_CUSTOM2
#if BUTTON_REMOTE != 0
        || context & CONTEXT_REMOTE
#endif
       )
    {
        return target_get_context_mapping(context & ~CONTEXT_CUSTOM2);
    }

    switch (context)
    {
        case CONTEXT_STD:
            return button_context_standard;
        case CONTEXT_WPS:
            return button_context_wps;

        case CONTEXT_LIST:
            return button_context_list;
        case CONTEXT_MAINMENU:
        case CONTEXT_TREE:
            if (global_settings.hold_lr_for_scroll_in_list)
                return button_context_listtree_scroll_without_combo;
            else
                return button_context_listtree_scroll_with_combo;
        case CONTEXT_CUSTOM|CONTEXT_TREE:
            return button_context_tree;

        case CONTEXT_SETTINGS:
            return button_context_settings;
        case CONTEXT_CUSTOM|CONTEXT_SETTINGS:
            return button_context_settings_right_is_inc;

        case CONTEXT_SETTINGS_COLOURCHOOSER:
            return button_context_colorchooser;
        case CONTEXT_SETTINGS_EQ:
            return button_context_eq;

        case CONTEXT_SETTINGS_TIME:
            return button_context_time;

        case CONTEXT_YESNOSCREEN:
            return button_context_yesno;
        case CONTEXT_FM:
            return button_context_radio;
        case CONTEXT_BOOKMARKSCREEN:
            return button_context_bmark;
        case CONTEXT_QUICKSCREEN:
            return button_context_quickscreen;
        case CONTEXT_PITCHSCREEN:
            return button_context_pitchscreen;
        case CONTEXT_KEYBOARD:
            return button_context_keyboard;
#ifdef USB_ENABLE_HID
        case CONTEXT_USB_HID:
            return button_context_usb_hid;
        case CONTEXT_USB_HID_MODE_MULTIMEDIA:
            return button_context_usb_hid_mode_multimedia;
        case CONTEXT_USB_HID_MODE_PRESENTATION:
            return button_context_usb_hid_mode_presentation;
        case CONTEXT_USB_HID_MODE_BROWSER:
            return button_context_usb_hid_mode_browser;
#ifdef HAVE_USB_HID_MOUSE
        case CONTEXT_USB_HID_MODE_MOUSE:
            return button_context_usb_hid_mode_mouse;
#endif
#endif
        default:
            return button_context_standard;
    }
    return button_context_standard;
}
