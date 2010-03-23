/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: main.c 11997 2007-01-13 09:08:18Z miipekk $
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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
#include "lcd.h"
#include "lcd-remote.h"
#include "font.h"
#include "system.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "cpu.h"
#include "common.h"
#include "power.h"
#include "kernel.h"
#include "config.h"
#include "logf.h"
#include "button.h"
#include "string.h"
#include "usb.h"
#include "file.h"

/* TODO: Other bootloaders need to be adjusted to set this variable to true
   on a button press - currently only the ipod, H10, Vibe 500 and Sansa versions do. */
#if defined(IPOD_ARCH) || defined(IRIVER_H10)  || defined(IRIVER_H10_5GB) \
 || defined(SANSA_E200) || defined(SANSA_C200) || defined(GIGABEAT_F) \
 || (CONFIG_CPU == AS3525) || defined(COWON_D2) \
 || defined(MROBE_100) || defined(MROBE_500) \
 || defined(SAMSUNG_YH925) || defined(SAMSUNG_YH920) \
 || defined(SAMSUNG_YH820) || defined(PHILIPS_SA9200) \
 || defined(PHILIPS_HDD1630) || defined(PHILIPS_HDD6330) \
 || defined(ONDA_VX747) || defined(SANSA_CLIPPLUS) \
 || defined(PBELL_VIBE500)
bool verbose = false;
#else
bool verbose = true;
#endif

int line = 0;
#ifdef HAVE_REMOTE_LCD
int remote_line = 0;
#endif

char printfbuf[256];

void reset_screen(void)
{
    lcd_clear_display();
    line = 0;
#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
    remote_line = 0;
#endif
}

void printf(const char *format, ...)
{
    int len;
    unsigned char *ptr;
    va_list ap;
    va_start(ap, format);

    ptr = printfbuf;
    len = vsnprintf(ptr, sizeof(printfbuf), format, ap);
    va_end(ap);

    lcd_puts(0, line++, ptr);
    if (verbose)
        lcd_update();
    if(line >= LCD_HEIGHT/SYSFONT_HEIGHT)
        line = 0;
#ifdef HAVE_REMOTE_LCD
    lcd_remote_puts(0, remote_line++, ptr);
    if (verbose)
        lcd_remote_update();
    if(remote_line >= LCD_REMOTE_HEIGHT/SYSFONT_HEIGHT)
        remote_line = 0;
#endif
}

char *strerror(int error)
{
    switch(error)
    {
    case EOK:
        return "OK";
    case EFILE_NOT_FOUND:
        return "File not found";
    case EREAD_CHKSUM_FAILED:
        return "Read failed (chksum)";
    case EREAD_MODEL_FAILED:
        return "Read failed (model)";
    case EREAD_IMAGE_FAILED:
        return "Read failed (image)";
    case EBAD_CHKSUM:
        return "Bad checksum";
    case EFILE_TOO_BIG:
        return "File too big";
    case EINVALID_FORMAT:
        return "Invalid file format";
    default:
        return "Unknown";
    }
}

void error(int errortype, int error)
{
    switch(errortype)
    {
    case EATA:
        printf("ATA error: %d", error);
        break;

    case EDISK:
        printf("No partition found");
        break;

    case EBOOTFILE:
        printf(strerror(error));
        break;
    }

    lcd_update();
    sleep(5*HZ);
    power_off();
}

/* Load firmware image in a format created by tools/scramble */
int load_firmware(unsigned char* buf, char* firmware, int buffer_size)
{
    int fd;
    int rc;
    int len;
    unsigned long chksum;
    char model[5];
    unsigned long sum;
    int i;
    char filename[MAX_PATH];

    snprintf(filename,sizeof(filename), BOOTDIR "/%s",firmware);
    fd = open(filename, O_RDONLY);
    if(fd < 0)
    {
        snprintf(filename,sizeof(filename),"/%s",firmware);
        fd = open(filename, O_RDONLY);
        if(fd < 0)
            return EFILE_NOT_FOUND;
    }

    len = filesize(fd) - 8;

    printf("Length: %x", len);

    if (len > buffer_size)
        return EFILE_TOO_BIG;

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);

    rc = read(fd, &chksum, 4);
    chksum=betoh32(chksum); /* Rockbox checksums are big-endian */
    if(rc < 4)
        return EREAD_CHKSUM_FAILED;

    printf("Checksum: %x", chksum);

    rc = read(fd, model, 4);
    if(rc < 4)
        return EREAD_MODEL_FAILED;

    model[4] = 0;

    printf("Model name: %s", model);
    printf("Loading %s", firmware);

    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    rc = read(fd, buf, len);
    if(rc < len)
        return EREAD_IMAGE_FAILED;

    close(fd);

    sum = MODEL_NUMBER;

    for(i = 0;i < len;i++) {
        sum += buf[i];
    }

    printf("Sum: %x", sum);

    if(sum != chksum)
        return EBAD_CHKSUM;

    return EOK;
}

/* Load raw binary image. */
int load_raw_firmware(unsigned char* buf, char* firmware, int buffer_size)
{
    int fd;
    int rc;
    int len;
    char filename[MAX_PATH];

    snprintf(filename,sizeof(filename),"%s",firmware);
    fd = open(filename, O_RDONLY);
    if(fd < 0)
    {
        return EFILE_NOT_FOUND;
    }

    len = filesize(fd);
    
    if (len > buffer_size)
        return EFILE_TOO_BIG;

    rc = read(fd, buf, len);
    if(rc < len)
        return EREAD_IMAGE_FAILED;

    close(fd);
    return len;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
int dbg_ports(void)
{
   return 0;
}

void mpeg_stop(void)
{
}

#ifdef ROCKBOX_HAS_LOGF /* Logf display helper for the bootloader */

#define LINES   (LCD_HEIGHT/SYSFONT_HEIGHT)
#define COLUMNS ((LCD_WIDTH/SYSFONT_WIDTH) > MAX_LOGF_ENTRY ? \
                            MAX_LOGF_ENTRY : (LCD_WIDTH/SYSFONT_WIDTH))

#ifdef ONDA_VX747
#define LOGF_UP     BUTTON_VOL_UP
#define LOGF_DOWN   BUTTON_VOL_DOWN
#define LOGF_CLEAR  BUTTON_MENU
#else
#warning No keymap defined for this target
#endif

void display_logf(void) /* Doesn't return! */
{
    int i, index, button, user_index=0;
#ifdef HAVE_TOUCHSCREEN
    int touch, prev_y=0;
#endif
    char buffer[COLUMNS+1];
    
    while(1)
    {
        index = logfindex + user_index;
        
        lcd_clear_display();
        for(i = LINES-1; i>=0; i--)
        {
            if(--index < 0)
            {
                if(logfwrap)
                    index = MAX_LOGF_LINES-1;
                else
                    break; /* done */
            }
            
            memcpy(buffer, logfbuffer[index], COLUMNS);
            
            if (logfbuffer[index][MAX_LOGF_ENTRY] == LOGF_TERMINATE_CONTINUE_LINE)
                buffer[MAX_LOGF_ENTRY-1] = '>';
            else if (logfbuffer[index][MAX_LOGF_ENTRY] == LOGF_TERMINATE_MULTI_LINE)
                buffer[MAX_LOGF_ENTRY-1] = '\0';
            
            buffer[COLUMNS] = '\0';
            
            lcd_puts(0, i, buffer);
        }
        
        button = button_get(false);
        if(button == SYS_USB_CONNECTED)
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
        else if(button == SYS_USB_DISCONNECTED)
            usb_acknowledge(SYS_USB_DISCONNECTED_ACK);
        else if(button & LOGF_UP)
            user_index++;
        else if(button & LOGF_DOWN)
            user_index--;
        else if(button & LOGF_CLEAR)
            user_index = 0;
#ifdef HAVE_TOUCHSCREEN
        else if(button & BUTTON_TOUCHSCREEN)
        {
            touch = button_get_data();
            
            if(button & BUTTON_REL)
                prev_y = 0;
            
            if(prev_y != 0)
                user_index += (prev_y - (touch & 0xFFFF)) / SYSFONT_HEIGHT;
            prev_y = touch & 0xFFFF;
        }
#endif
        
        lcd_update();
        sleep(HZ/16);
    }
}
#endif
