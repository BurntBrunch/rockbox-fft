/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Uwe Freese
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
#include "config.h"

#include <stdbool.h>

#include "lcd.h"
#include "action.h"
#include "kernel.h"
#include "sprintf.h"
#include <string.h>
#include "settings.h"
#include "power.h"
#include "icons.h"
#include "rtc.h"
#include "misc.h"
#include "screens.h"
#include "talk.h"
#include "lang.h"
#include "alarm_menu.h"
#include "splash.h"
#include "viewport.h"

static void speak_time(int hours, int minutes, bool speak_hours, bool enqueue)
{
    if (global_settings.talk_menu){
        if(speak_hours) {
            talk_value(hours, UNIT_HOUR, enqueue);
            talk_value(minutes, UNIT_MIN, true);
        } else {
            talk_value(minutes, UNIT_MIN, enqueue);
        }
    }
}

bool alarm_screen(void)
{
    int h, m;
    bool done = false;
    struct tm *tm;
    int togo;
    int button;
    int i;
    bool update = true;
    bool hour_wrapped = false;
    struct viewport vp[NB_SCREENS];

    rtc_get_alarm(&h, &m);

    /* After a battery change the RTC values are out of range */
    if (m > 60 || h > 24) {
        m = 0;
        h = 12;
    } else {
        m = m / 5 * 5; /* 5 min accuracy should be enough */
    }
    FOR_NB_SCREENS(i)
    {
        viewport_set_defaults(&vp[i], i);
    }

    while(!done) {
        if(update)
        {
            FOR_NB_SCREENS(i)
            {
                screens[i].set_viewport(&vp[i]);
                screens[i].clear_viewport();
                screens[i].puts(0, 3, str(LANG_ALARM_MOD_KEYS));
            }
            /* Talk when entering the wakeup screen */
            speak_time(h, m, true, true);
            update = false;
        }

        FOR_NB_SCREENS(i)
        {
            screens[i].set_viewport(&vp[i]);
            screens[i].putsf(0, 1, str(LANG_ALARM_MOD_TIME), h, m);
            screens[i].update_viewport();
            screens[i].set_viewport(NULL);
        }
        button = get_action(CONTEXT_SETTINGS,HZ);

        switch(button) {
            case ACTION_STD_OK:
            /* prevent that an alarm occurs in the shutdown procedure */
            /* accept alarms only if they are in 2 minutes or more */
            tm = get_time();
            togo = (m + h * 60 - tm->tm_min - tm->tm_hour * 60 + 1440) % 1440;

            if (togo > 1) {
                rtc_init();
                rtc_set_alarm(h,m);
                rtc_enable_alarm(true);
                if (global_settings.talk_menu)
                {
                    talk_id(LANG_ALARM_MOD_TIME_TO_GO, true);
                    talk_value(togo / 60, UNIT_HOUR, true);
                    talk_value(togo % 60, UNIT_MIN, true);
                    talk_force_enqueue_next();
                }
                splashf(HZ*2, str(LANG_ALARM_MOD_TIME_TO_GO),
                               togo / 60, togo % 60);
                done = true;
            } else {
                splash(HZ, ID2P(LANG_ALARM_MOD_ERROR));
                update = true;
            }
            break;

         /* inc(m) */
        case ACTION_SETTINGS_INC:
        case ACTION_SETTINGS_INCREPEAT:
            m += 5;
            if (m == 60) {
                h += 1;
                m = 0;
                hour_wrapped = true;
            }
            if (h == 24)
                h = 0;

            speak_time(h, m, hour_wrapped, false);
            break;

         /* dec(m) */
        case ACTION_SETTINGS_DEC:
        case ACTION_SETTINGS_DECREPEAT:
             m -= 5;
             if (m == -5) {
                 h -= 1;
                 m = 55;
                 hour_wrapped = true;
             }
             if (h == -1)
                 h = 23;

             speak_time(h, m, hour_wrapped, false);
             break;

         /* inc(h) */
         case ACTION_STD_NEXT:
         case ACTION_STD_NEXTREPEAT:
             h = (h+1) % 24;

             if (global_settings.talk_menu)
                 talk_value(h, UNIT_HOUR, false);
             break;

         /* dec(h) */
        case ACTION_STD_PREV:
        case ACTION_STD_PREVREPEAT:
             h = (h+23) % 24;
             
             if (global_settings.talk_menu)
                 talk_value(h, UNIT_HOUR, false);
             break;

        case ACTION_STD_CANCEL:
            rtc_enable_alarm(false);
            splash(HZ*2, ID2P(LANG_ALARM_MOD_DISABLE));
            done = true;
            break;

        case ACTION_NONE:
            hour_wrapped = false;
            break;

        default:
            if(default_event_handler(button) == SYS_USB_CONNECTED)
            {
                rtc_enable_alarm(false);
                return true;
            }
            break;
        }
    }
    return false;
}
