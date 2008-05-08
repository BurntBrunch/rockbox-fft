/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "system.h"
#include "power.h"
#include "backlight.h"
#include "backlight-target.h"
#include "avic-imx31.h"
#include "mc13783.h"

#ifndef SIMULATOR

static bool charger_detect = false;

void power_init(void)
{
}

/* This is called from the mc13783 interrupt thread */
void set_charger_inserted(bool inserted)
{
    charger_detect = inserted;
}

bool charger_inserted(void)
{
    return charger_detect;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void)
{
    return false;
}

void ide_power_enable(bool on)
{
    if (!on)
    {
        /* Bus must be isolated before power off */
        imx31_regmod32(&GPIO2_DR, (1 << 16), (1 << 16));
    }

    /* HD power switch */
    imx31_regmod32(&GPIO3_DR, on ? (1 << 5) : 0, (1 << 5));

    if (on)
    {
        /* Bus switch may be turned on after powerup */
        sleep(HZ/10);
        imx31_regmod32(&GPIO2_DR, 0, (1 << 16));
    }
}

bool ide_powered(void)
{
    return (GPIO3_DR & (1 << 5)) != 0;
}

void power_off(void)
{
    mc13783_set(MC13783_POWER_CONTROL0, MC13783_USEROFFSPI);

    disable_interrupt(IRQ_FIQ_STATUS);
    while (1);
}

#else /* SIMULATOR */

bool charger_inserted(void)
{
    return false;
}

void charger_enable(bool on)
{
    (void)on;
}

void power_off(void)
{
}

void ide_power_enable(bool on)
{
   (void)on;
}

#endif /* SIMULATOR */

