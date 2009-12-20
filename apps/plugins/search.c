/***************************************************************************
 *
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Stefan Meyer
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
#include "ctype.h"

PLUGIN_HEADER

#define BUFFER_SIZE 16384

static int fd;
static int fdw;

static int file_size, bomsize;
static int results = 0;

static char buffer[BUFFER_SIZE+1];
static char search_string[60] ;

static int buffer_pos; /* Position of the buffer in the file */

static int line_end;   /* Index of the end of line */

char resultfile[MAX_PATH];
char path[MAX_PATH];

static int strpcasecmp(const char *s1, const char *s2){
    while (*s1 != '\0' && tolower(*s1) == tolower(*s2)) {
        s1++;
        s2++;
    }

    return (*s1 == '\0');
}

static void fill_buffer(int pos){
    int numread;
    int i;
    int found = false ;
    const char crlf = '\n';

    rb->lseek(fd, pos+bomsize, SEEK_SET);
    numread = rb->read(fd, buffer, MIN(BUFFER_SIZE, file_size-pos));

    buffer[numread] = 0;
    line_end = 0;

    for(i=0;i<numread;i++) {
        switch(buffer[i]) {
            case '\r':
                buffer[i] = ' ';
                break;
            case '\n':
                buffer[i] = 0;
                buffer_pos = pos + i +1 ;

                if (found){
                    /* write to playlist */
                    rb->write(fdw, &buffer[line_end],
                              rb->strlen( &buffer[line_end] ));
                    rb->write(fdw, &crlf, 1);

                    found = false ;
                    results++ ;
                }
                line_end = i +1 ;

                break;

            default:
                if (!found && tolower(buffer[i]) == tolower(search_string[0]))
                    found = strpcasecmp(&search_string[0],&buffer[i]) ;
                break;
        }
    }
    DEBUGF("\n-------------------\n");
}

static void search_buffer(void){
    buffer_pos = 0;

    fill_buffer(0);
    while ((buffer_pos+1) < file_size)
        fill_buffer(buffer_pos);
}

static void clear_display(void){
    int i;
    FOR_NB_SCREENS(i){
        rb->screens[i]->clear_display();
    }
}

static bool search_init(const char* file){
    rb->memset(search_string, 0, sizeof(search_string));

    if (!rb->kbd_input(search_string,sizeof search_string)){
        clear_display();
        rb->splash(0, "Searching...");
        fd = rb->open_utf8(file, O_RDONLY);
        if (fd < 0)
            return false;

        bomsize = rb->lseek(fd, 0, SEEK_CUR);
        if (bomsize)
            fdw = rb->open_utf8(resultfile, O_WRONLY|O_CREAT|O_TRUNC);
        else
            fdw = rb->open(resultfile, O_WRONLY|O_CREAT|O_TRUNC);

        if (fdw < 0) {
#ifdef HAVE_LCD_BITMAP
            rb->splash(HZ, "Failed to create result file!");
#else
            rb->splash(HZ, "File creation failed");
#endif
            rb->close(fd);
            return false;
        }

        file_size = rb->lseek(fd, 0, SEEK_END) - bomsize;

        return true;
    }

    return false ;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    int ok;
    const char *filename = parameter;
    char *p;
    if(!parameter) return PLUGIN_ERROR;

    DEBUGF("%s - %s\n", (char *)parameter, &filename[rb->strlen(filename)-4]);
    /* Check the extension. We only allow .m3u files. */
    if (!(p = rb->strrchr(filename, '.')) ||
        (rb->strcasecmp(p, ".m3u") && rb->strcasecmp(p, ".m3u8")))
    {
        rb->splash(HZ, "Not a .m3u or .m3u8 file");
        return PLUGIN_ERROR;
    }

    rb->strcpy(path, filename);

    p = rb->strrchr(path, '/');
    if(p)
        *p = 0;

    rb->snprintf(resultfile, MAX_PATH, "%s/search_result.m3u", path);
    ok = search_init(parameter);
    if (!ok)
        return PLUGIN_ERROR;
    search_buffer();

    clear_display();
    rb->splash(HZ, "Done");
    rb->close(fdw);
    rb->close(fd);
    rb->reload_directory();

    return PLUGIN_OK;
}
