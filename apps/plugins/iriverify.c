/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Alexandre Bourget
 *
 * Plugin to transform a Rockbox produced m3u playlist into something
 * understandable by the picky original iRiver firmware.
 *
 * Based on sort.c by the Rockbox team.
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

PLUGIN_HEADER

ssize_t buf_size;
static char *filename;
static int readsize;
static char *stringbuffer;
static char crlf[2] = "\r\n";

int read_buffer(int offset)
{
    int fd;
    
    fd = rb->open(filename, O_RDONLY);
    if(fd < 0)
        return 10 * fd - 1;

    /* Fill the buffer from the file */
    rb->lseek(fd, offset, SEEK_SET);
    readsize = rb->read(fd, stringbuffer, buf_size);
    rb->close(fd);

    if(readsize < 0)
        return readsize * 10 - 2;

    if(readsize == buf_size)
        return buf_size; /* File too big */

    return 0;
}

static int write_file(void)
{
    char tmpfilename[MAX_PATH+1];
    int fd;
    int rc;
    char *buf_ptr;
    char *str_begin;

    /* Create a temporary file */
    
    rb->snprintf(tmpfilename, MAX_PATH+1, "%s.tmp", filename);
    
    fd = rb->creat(tmpfilename);
    if(fd < 0)
        return 10 * fd - 1;

    /* Let's make sure it always writes CR/LF and not only LF */
    buf_ptr = stringbuffer;
    str_begin = stringbuffer;
    do {
    /* Transform slashes into backslashes */
        if(*buf_ptr == '/')
        *buf_ptr = '\\';

    if((*buf_ptr == '\r') || (*buf_ptr == '\n')) {
        /* We have no complete string ? It's only a leading \n or \r ? */
        if (!str_begin)
        continue;
        
        /* Terminate string */
        *buf_ptr = 0;

        /* Write our new string */
        rc = rb->write(fd, str_begin, rb->strlen(str_begin));
        if(rc < 0) {
        rb->close(fd);
        return 10 * rc - 2;
        }
        /* Write CR/LF */
        rc = rb->write(fd, crlf, 2);
        if(rc < 0) {
        rb->close(fd);
        return 10 * rc - 3;
        }

        /* Reset until we get a new line */
        str_begin = NULL;

    }
    else {
        /* We start a new line here */
        if (!str_begin)
        str_begin = buf_ptr;
    }

    /* Next char, until ... */
    } while(buf_ptr++ < stringbuffer + readsize);

    rb->close(fd);

    /* Remove the original file */
    rc = rb->remove(filename);
    if(rc < 0) {
        return 10 * rc - 4;
    }

    /* Replace the old file with the new */
    rc = rb->rename(tmpfilename, filename);
    if(rc < 0) {
        return 10 * rc - 5;
    }
    
    return 0;
}

enum plugin_status plugin_start(const void* parameter)
{
    char *buf;
    int rc;
    int i;
    if(!parameter) return PLUGIN_ERROR;
    filename = (char *)parameter;

    buf = rb->plugin_get_audio_buffer((size_t *)&buf_size); /* start munching memory */

    stringbuffer = buf;

    FOR_NB_SCREENS(i)
        rb->screens[i]->clear_display();
    rb->splash(0, "Converting...");
    
    rc = read_buffer(0);
    FOR_NB_SCREENS(i)
        rb->screens[i]->clear_display();
    if(rc == 0) {
        rb->splash(0, "Writing...");
        rc = write_file();

        FOR_NB_SCREENS(i)
            rb->screens[i]->clear_display();
        if(rc < 0) {
            rb->splashf(HZ, "Can't write file: %d", rc);
        } else {
            rb->splash(HZ, "Done");
        }
    } else {
        if(rc < 0) {
            rb->splashf(HZ, "Can't read file: %d", rc);
        } else {
            rb->splash(HZ, "The file is too big");
        }
    }

    return PLUGIN_OK;
}
