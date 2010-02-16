/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Miika Pekkarinen
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
#ifndef _DIRCACHE_H
#define _DIRCACHE_H

#include "dir_uncached.h"

#ifdef HAVE_DIRCACHE

#define DIRCACHE_RESERVE  (1024*64)
#define DIRCACHE_LIMIT    (1024*1024*6)
#define DIRCACHE_FILE     ROCKBOX_DIR"/dircache.dat"

#define DIRCACHE_APPFLAG_TAGCACHE  0x0001

/* Internal structures. */
struct travel_data {
    struct dircache_entry *first;
    struct dircache_entry *ce;
    struct dircache_entry *down_entry;
#ifdef SIMULATOR
    DIR_UNCACHED *dir, *newdir;
    struct dirent_uncached *entry;
#else
    struct fat_dir *dir;
    struct fat_dir newdir;
    struct fat_direntry entry;
#endif
    int pathpos;
};

#define DIRCACHE_MAGIC  0x00d0c0a0
struct dircache_maindata {
    long magic;
    long size;
    long entry_count;
    long appflags;
    struct dircache_entry *root_entry;
};

#define MAX_PENDING_BINDINGS 2
struct fdbind_queue {
    char path[MAX_PATH];
    int fd;
};

/* Exported structures. */
struct dircache_entry {
    struct dircache_entry *next;
    struct dircache_entry *up;
    struct dircache_entry *down;
    int attribute;
    long size;
    long startcluster;
    unsigned short wrtdate;
    unsigned short wrttime;
    unsigned long name_len;
    char *d_name;
};

struct dirent_cached {
    char *d_name;
    int attribute;
    long size;
    long startcluster;
    unsigned short wrtdate; /*  Last write date */ 
    unsigned short wrttime; /*  Last write time */
};

typedef struct {
    bool busy;
    struct dirent_cached theent; /* .attribute is set to -1 on init(opendir) */
    /* the two following field can't be used at the same time so have an union */
    struct dircache_entry *internal_entry; /* the current entry in the directory */
    DIR_UNCACHED *regulardir;
} DIR_CACHED;

void dircache_init(void);
int dircache_load(void);
int dircache_save(void);
int dircache_build(int last_size);
void* dircache_steal_buffer(long *size);
bool dircache_is_enabled(void);
bool dircache_is_initializing(void);
void dircache_set_appflag(long mask);
bool dircache_get_appflag(long mask);
int dircache_get_entry_count(void);
int dircache_get_cache_size(void);
int dircache_get_reserve_used(void);
int dircache_get_build_ticks(void);
void dircache_disable(void);
const struct dircache_entry *dircache_get_entry_ptr(const char *filename);
void dircache_copy_path(const struct dircache_entry *entry, char *buf, int size);

void dircache_bind(int fd, const char *path);
void dircache_update_filesize(int fd, long newsize, long startcluster);
void dircache_update_filetime(int fd);
void dircache_mkdir(const char *path);
void dircache_rmdir(const char *path);
void dircache_remove(const char *name);
void dircache_rename(const char *oldpath, const char *newpath);
void dircache_add_file(const char *path, long startcluster);

DIR_CACHED* opendir_cached(const char* name);
struct dirent_cached* readdir_cached(DIR_CACHED* dir);
int closedir_cached(DIR_CACHED *dir);
int mkdir_cached(const char *name);
int rmdir_cached(const char* name);
#endif /* !HAVE_DIRCACHE */

#endif
