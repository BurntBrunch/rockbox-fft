/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "file.h"
#include "fat.h"
#include "dir_uncached.h"
#include "debug.h"
#include "dircache.h"
#include "filefuncs.h"
#include "system.h"

/*
  These functions provide a roughly POSIX-compatible file IO API.

  Since the fat32 driver only manages sectors, we maintain a one-sector
  cache for each open file. This way we can provide byte access without
  having to re-read the sector each time. 
  The penalty is the RAM used for the cache and slightly more complex code.
*/

struct filedesc {
    unsigned char cache[SECTOR_SIZE];
    int cacheoffset; /* invariant: 0 <= cacheoffset <= SECTOR_SIZE */
    long fileoffset;
    long size;
    int attr;
    struct fat_file fatfile;
    bool busy;
    bool write;
    bool dirty;
    bool trunc;
};

static struct filedesc openfiles[MAX_OPEN_FILES];

static int flush_cache(int fd);

int creat(const char *pathname)
{
    return open(pathname, O_WRONLY|O_CREAT|O_TRUNC);
}

static int open_internal(const char* pathname, int flags, bool use_cache)
{
    DIR_UNCACHED* dir;
    struct dirent_uncached* entry;
    int fd;
    char pathnamecopy[MAX_PATH];
    char* name;
    struct filedesc* file = NULL;
    int rc;
#ifndef HAVE_DIRCACHE
    (void)use_cache;
#endif

    LDEBUGF("open(\"%s\",%d)\n",pathname,flags);

    if ( pathname[0] != '/' ) {
        DEBUGF("'%s' is not an absolute path.\n",pathname);
        DEBUGF("Only absolute pathnames supported at the moment\n");
        errno = EINVAL;
        return -1;
    }

    /* find a free file descriptor */
    for ( fd=0; fd<MAX_OPEN_FILES; fd++ )
        if ( !openfiles[fd].busy )
            break;

    if ( fd == MAX_OPEN_FILES ) {
        DEBUGF("Too many files open\n");
        errno = EMFILE;
        return -2;
    }

    file = &openfiles[fd];
    memset(file, 0, sizeof(struct filedesc));

    if (flags & (O_RDWR | O_WRONLY)) {
        file->write = true;

        if (flags & O_TRUNC)
            file->trunc = true;
    }
    file->busy = true;

#ifdef HAVE_DIRCACHE
    if (dircache_is_enabled() && !file->write && use_cache)
    {
        const struct dircache_entry *ce;
# ifdef HAVE_MULTIVOLUME
        int volume = strip_volume(pathname, pathnamecopy);
# endif

        ce = dircache_get_entry_ptr(pathname);
        if (!ce)
        {
            errno = ENOENT;
            file->busy = false;
            return -7;
        }

        fat_open(IF_MV2(volume,)
                 ce->startcluster,
                 &(file->fatfile),
                 NULL);
        file->size = ce->size;
        file->attr = ce->attribute;
        file->cacheoffset = -1;
        file->fileoffset = 0;

        return fd;
    }
#endif

    strlcpy(pathnamecopy, pathname, sizeof(pathnamecopy));

    /* locate filename */
    name=strrchr(pathnamecopy+1,'/');
    if ( name ) {
        *name = 0; 
        dir = opendir_uncached(pathnamecopy);
        *name = '/';
        name++;
    }
    else {
        dir = opendir_uncached("/");
        name = pathnamecopy+1;
    }
    if (!dir) {
        DEBUGF("Failed opening dir\n");
        errno = EIO;
        file->busy = false;
        return -4;
    }

    if(name[0] == 0) {
        DEBUGF("Empty file name\n");
        errno = EINVAL;
        file->busy = false;
        closedir_uncached(dir);
        return -5;
    }

    /* scan dir for name */
    while ((entry = readdir_uncached(dir))) {
        if ( !strcasecmp(name, entry->d_name) ) {
            fat_open(IF_MV2(dir->fatdir.file.volume,)
                     entry->startcluster,
                     &(file->fatfile),
                     &(dir->fatdir));
            file->size = file->trunc ? 0 : entry->size;
            file->attr = entry->attribute;
            break;
        }
    }

    if ( !entry ) {
        LDEBUGF("Didn't find file %s\n",name);
        if ( file->write && (flags & O_CREAT) ) {
            rc = fat_create_file(name,
                                 &(file->fatfile),
                                 &(dir->fatdir));
            if (rc < 0) {
                DEBUGF("Couldn't create %s in %s\n",name,pathnamecopy);
                errno = EIO;
                file->busy = false;
                closedir_uncached(dir);
                return rc * 10 - 6;
            }
#ifdef HAVE_DIRCACHE
            dircache_add_file(pathname, file->fatfile.firstcluster);
#endif
            file->size = 0;
            file->attr = 0;
        }
        else {
            DEBUGF("Couldn't find %s in %s\n",name,pathnamecopy);
            errno = ENOENT;
            file->busy = false;
            closedir_uncached(dir);
            return -7;
        }
    } else {
        if(file->write && (file->attr & FAT_ATTR_DIRECTORY)) {
            errno = EISDIR;
            file->busy = false;
            closedir_uncached(dir);
            return -8;
        }
    }
    closedir_uncached(dir);

    file->cacheoffset = -1;
    file->fileoffset = 0;

    if (file->write && (flags & O_APPEND)) {
        rc = lseek(fd,0,SEEK_END);
        if (rc < 0 )
            return rc * 10 - 9;
    }

#ifdef HAVE_DIRCACHE
    if (file->write)
        dircache_bind(fd, pathname);
#endif

    return fd;
}

int open(const char* pathname, int flags)
{
    /* By default, use the dircache if available. */
    return open_internal(pathname, flags, true);
}

int close(int fd)
{
    struct filedesc* file = &openfiles[fd];
    int rc = 0;

    LDEBUGF("close(%d)\n", fd);

    if (fd < 0 || fd > MAX_OPEN_FILES-1) {
        errno = EINVAL;
        return -1;
    }
    if (!file->busy) {
        errno = EBADF;
        return -2;
    }
    if (file->write) {
        rc = fsync(fd);
        if (rc < 0)
            return rc * 10 - 3;
#ifdef HAVE_DIRCACHE
        dircache_update_filesize(fd, file->size, file->fatfile.firstcluster);
        dircache_update_filetime(fd);
#endif
    }

    file->busy = false;
    return 0;
}

int fsync(int fd)
{
    struct filedesc* file = &openfiles[fd];
    int rc = 0;

    LDEBUGF("fsync(%d)\n", fd);

    if (fd < 0 || fd > MAX_OPEN_FILES-1) {
        errno = EINVAL;
        return -1;
    }
    if (!file->busy) {
        errno = EBADF;
        return -2;
    }
    if (file->write) {
        /* flush sector cache */
        if ( file->dirty ) {
            rc = flush_cache(fd);
            if (rc < 0)
            {
                /* when failing, try to close the file anyway */
                fat_closewrite(&(file->fatfile), file->size, file->attr);
                return rc * 10 - 3;
            }
        }

        /* truncate? */
        if (file->trunc) {
            rc = ftruncate(fd, file->size);
            if (rc < 0)
            {
                /* when failing, try to close the file anyway */
                fat_closewrite(&(file->fatfile), file->size, file->attr);
                return rc * 10 - 4;
            }
        }

        /* tie up all loose ends */
        rc = fat_closewrite(&(file->fatfile), file->size, file->attr);
        if (rc < 0)
            return rc * 10 - 5;
    }
    return 0;
}

int remove(const char* name)
{
    int rc;
    struct filedesc* file;
    /* Can't use dircache now, because we need to access the fat structures. */
    int fd = open_internal(name, O_WRONLY, false);
    if ( fd < 0 )
        return fd * 10 - 1;

    file = &openfiles[fd];
#ifdef HAVE_DIRCACHE
    dircache_remove(name);
#endif
    rc = fat_remove(&(file->fatfile));
    if ( rc < 0 ) {
        DEBUGF("Failed removing file: %d\n", rc);
        errno = EIO;
        return rc * 10 - 3;
    }

    file->size = 0;

    rc = close(fd);
    if (rc<0)
        return rc * 10 - 4;

    return 0;
}

int rename(const char* path, const char* newpath)
{
    int rc, fd;
    DIR_UNCACHED* dir;
    char* nameptr;
    char* dirptr;
    struct filedesc* file;
    char newpath2[MAX_PATH];

    /* verify new path does not already exist */
    /* If it is a directory, errno == EISDIR if the name exists */
    fd = open(newpath, O_RDONLY);
    if ( fd >= 0 || errno == EISDIR) {
        close(fd);
        errno = EBUSY;
        return -1;
    }
    close(fd);

    fd = open_internal(path, O_RDONLY, false);
    if ( fd < 0 ) {
        errno = EIO;
        return fd * 10 - 2;
    }

    /* extract new file name */
    nameptr = strrchr(newpath,'/');
    if (nameptr)
        nameptr++;
    else
        return - 3;

    /* Extract new path */
    strcpy(newpath2, newpath);

    dirptr = strrchr(newpath2,'/');
    if(dirptr)
        *dirptr = 0;
    else
        return - 4;

    dirptr = newpath2;

    if(strlen(dirptr) == 0) {
        dirptr = "/";
    }

    dir = opendir_uncached(dirptr);
    if(!dir)
        return - 5;

    file = &openfiles[fd];

    rc = fat_rename(&file->fatfile, &dir->fatdir, nameptr,
                    file->size, file->attr);
#ifdef HAVE_MULTIVOLUME
    if ( rc == -1) {
        DEBUGF("Failed renaming file across volumnes: %d\n", rc);
        errno = EXDEV;
        return -6;
    }
#endif
    if ( rc < 0 ) {
        DEBUGF("Failed renaming file: %d\n", rc);
        errno = EIO;
        return rc * 10 - 7;
    }

#ifdef HAVE_DIRCACHE
    dircache_rename(path, newpath);
#endif

    rc = close(fd);
    if (rc<0) {
        errno = EIO;
        return rc * 10 - 8;
    }

    rc = closedir_uncached(dir);
    if (rc<0) {
        errno = EIO;
        return rc * 10 - 9;
    }

    return 0;
}

int ftruncate(int fd, off_t size)
{
    int rc, sector;
    struct filedesc* file = &openfiles[fd];

    sector = size / SECTOR_SIZE;
    if (size % SECTOR_SIZE)
        sector++;

    rc = fat_seek(&(file->fatfile), sector);
    if (rc < 0) {
        errno = EIO;
        return rc * 10 - 1;
    }

    rc = fat_truncate(&(file->fatfile));
    if (rc < 0) {
        errno = EIO;
        return rc * 10 - 2;
    }

    file->size = size;
#ifdef HAVE_DIRCACHE
    dircache_update_filesize(fd, size, file->fatfile.firstcluster);
#endif

    return 0;
}

static int flush_cache(int fd)
{
    int rc;
    struct filedesc* file = &openfiles[fd];
    long sector = file->fileoffset / SECTOR_SIZE;

    DEBUGF("Flushing dirty sector cache\n");

    /* make sure we are on correct sector */
    rc = fat_seek(&(file->fatfile), sector);
    if ( rc < 0 )
        return rc * 10 - 3;

    rc = fat_readwrite(&(file->fatfile), 1, file->cache, true );

    if ( rc < 0 ) {
        if(file->fatfile.eof)
            errno = ENOSPC;

        return rc * 10 - 2;
    }

    file->dirty = false;

    return 0;
}

static int readwrite(int fd, void* buf, long count, bool write)
{
    long sectors;
    long nread=0;
    struct filedesc* file;
    int rc;

    if (fd < 0 || fd > MAX_OPEN_FILES-1) {
        errno = EINVAL;
        return -1;
    }

    file = &openfiles[fd];

    if ( !file->busy ) {
        errno = EBADF;
        return -1;
    }

    if(file->attr & FAT_ATTR_DIRECTORY) {
        errno = EISDIR;
        return -1;
    }

    LDEBUGF( "readwrite(%d,%lx,%ld,%s)\n",
             fd,(long)buf,count,write?"write":"read");

    /* attempt to read past EOF? */
    if (!write && count > file->size - file->fileoffset)
        count = file->size - file->fileoffset;

    /* any head bytes? */
    if ( file->cacheoffset != -1 ) {
        int offs = file->cacheoffset;
        int headbytes = MIN(count, SECTOR_SIZE - offs);

        if (write) {
            memcpy( file->cache + offs, buf, headbytes );
            file->dirty = true;
        }
        else {
            memcpy( buf, file->cache + offs, headbytes );
        }

        if (offs + headbytes == SECTOR_SIZE) {
            if (file->dirty) {
                rc = flush_cache(fd);
                if ( rc < 0 ) {
                    errno = EIO;
                    return rc * 10 - 2;
                }
            }
            file->cacheoffset = -1;
        }
        else {
            file->cacheoffset += headbytes;
        }

        nread = headbytes;
        count -= headbytes;
    }

    /* If the buffer has been modified, either it has been flushed already
     * (if (offs+headbytes == SECTOR_SIZE)...) or does not need to be (no
     * more data to follow in this call). Do NOT flush here. */

    /* read/write whole sectors right into/from the supplied buffer */
    sectors = count / SECTOR_SIZE;
    if ( sectors ) {
        rc = fat_readwrite(&(file->fatfile), sectors,
            (unsigned char*)buf+nread, write );
        if ( rc < 0 ) {
            DEBUGF("Failed read/writing %ld sectors\n",sectors);
            errno = EIO;
            if(write && file->fatfile.eof) {
                DEBUGF("No space left on device\n");
                errno = ENOSPC;
            } else {
                file->fileoffset += nread;
            }
            file->cacheoffset = -1;
            /* adjust file size to length written */
            if ( write && file->fileoffset > file->size )
            {
                file->size = file->fileoffset;
#ifdef HAVE_DIRCACHE
                dircache_update_filesize(fd, file->size, file->fatfile.firstcluster);
#endif
            }
            return nread ? nread : rc * 10 - 4;
        }
        else {
            if ( rc > 0 ) {
                nread += rc * SECTOR_SIZE;
                count -= sectors * SECTOR_SIZE;

                /* if eof, skip tail bytes */
                if ( rc < sectors )
                    count = 0;
            }
            else {
                /* eof */
                count=0;
            }

            file->cacheoffset = -1;
        }
    }

    /* any tail bytes? */
    if ( count ) {
        if (write) {
            if ( file->fileoffset + nread < file->size ) {
                /* sector is only partially filled. copy-back from disk */
                LDEBUGF("Copy-back tail cache\n");
                rc = fat_readwrite(&(file->fatfile), 1, file->cache, false );
                if ( rc < 0 ) {
                    DEBUGF("Failed writing\n");
                    errno = EIO;
                    file->fileoffset += nread;
                    file->cacheoffset = -1;
                    /* adjust file size to length written */
                    if ( file->fileoffset > file->size )
                    {
                        file->size = file->fileoffset;
#ifdef HAVE_DIRCACHE
                        dircache_update_filesize(fd, file->size, file->fatfile.firstcluster);
#endif
                    }
                    return nread ? nread : rc * 10 - 5;
                }
                /* seek back one sector to put file position right */
                rc = fat_seek(&(file->fatfile), 
                              (file->fileoffset + nread) /
                              SECTOR_SIZE);
                if ( rc < 0 ) {
                    DEBUGF("fat_seek() failed\n");
                    errno = EIO;
                    file->fileoffset += nread;
                    file->cacheoffset = -1;
                    /* adjust file size to length written */
                    if ( file->fileoffset > file->size )
                    {
                        file->size = file->fileoffset;
#ifdef HAVE_DIRCACHE
                        dircache_update_filesize(fd, file->size, file->fatfile.firstcluster);
#endif
                    }
                    return nread ? nread : rc * 10 - 6;
                }
            }
            memcpy( file->cache, (unsigned char*)buf + nread, count );
            file->dirty = true;
        }
        else {
            rc = fat_readwrite(&(file->fatfile), 1, &(file->cache),false);
            if (rc < 1 ) {
                DEBUGF("Failed caching sector\n");
                errno = EIO;
                file->fileoffset += nread;
                file->cacheoffset = -1;
                return nread ? nread : rc * 10 - 7;
            }
            memcpy( (unsigned char*)buf + nread, file->cache, count );
        }

        nread += count;
        file->cacheoffset = count;
    }

    file->fileoffset += nread;
    LDEBUGF("fileoffset: %ld\n", file->fileoffset);

    /* adjust file size to length written */
    if ( write && file->fileoffset > file->size )
    {
        file->size = file->fileoffset;
#ifdef HAVE_DIRCACHE
        dircache_update_filesize(fd, file->size, file->fatfile.firstcluster);
#endif
    }

    return nread;
}

ssize_t write(int fd, const void* buf, size_t count)
{
    if (!openfiles[fd].write) {
        errno = EACCES;
        return -1;
    }
    return readwrite(fd, (void *)buf, count, true);
}

ssize_t read(int fd, void* buf, size_t count)
{
    return readwrite(fd, buf, count, false);
}


off_t lseek(int fd, off_t offset, int whence)
{
    off_t pos;
    long newsector;
    long oldsector;
    int sectoroffset;
    int rc;
    struct filedesc* file = &openfiles[fd];

    LDEBUGF("lseek(%d,%ld,%d)\n",fd,offset,whence);

    if (fd < 0 || fd > MAX_OPEN_FILES-1) {
        errno = EINVAL;
        return -1;
    }
    if ( !file->busy ) {
        errno = EBADF;
        return -1;
    }

    switch ( whence ) {
        case SEEK_SET:
            pos = offset;
            break;

        case SEEK_CUR:
            pos = file->fileoffset + offset;
            break;

        case SEEK_END:
            pos = file->size + offset;
            break;

        default:
            errno = EINVAL;
            return -2;
    }
    if ((pos < 0) || (pos > file->size)) {
        errno = EINVAL;
        return -3;
    }

    /* new sector? */
    newsector = pos / SECTOR_SIZE;
    oldsector = file->fileoffset / SECTOR_SIZE;
    sectoroffset = pos % SECTOR_SIZE;

    if ( (newsector != oldsector) ||
         ((file->cacheoffset==-1) && sectoroffset) ) {

        if ( newsector != oldsector ) {
            if (file->dirty) {
                rc = flush_cache(fd);
                if (rc < 0)
                    return rc * 10 - 5;
            }

            rc = fat_seek(&(file->fatfile), newsector);
            if ( rc < 0 ) {
                errno = EIO;
                return rc * 10 - 4;
            }
        }
        if ( sectoroffset ) {
            rc = fat_readwrite(&(file->fatfile), 1,
                               &(file->cache),false);
            if ( rc < 0 ) {
                errno = EIO;
                return rc * 10 - 6;
            }
            file->cacheoffset = sectoroffset;
        }
        else
            file->cacheoffset = -1;
    }
    else
        if ( file->cacheoffset != -1 )
            file->cacheoffset = sectoroffset;

    file->fileoffset = pos;

    return pos;
}

off_t filesize(int fd)
{
    struct filedesc* file = &openfiles[fd];

    if (fd < 0 || fd > MAX_OPEN_FILES-1) {
        errno = EINVAL;
        return -1;
    }
    if ( !file->busy ) {
        errno = EBADF;
        return -1;
    }

    return file->size;
}


#ifdef HAVE_HOTSWAP
/* release all file handles on a given volume "by force", to avoid leaks */
int release_files(int volume)
{
    struct filedesc* pfile = openfiles;
    int fd;
    int closed = 0;
    for ( fd=0; fd<MAX_OPEN_FILES; fd++, pfile++)
    {
#ifdef HAVE_MULTIVOLUME
        if (pfile->fatfile.volume == volume)
#else
        (void)volume;
#endif
        {
            pfile->busy = false; /* mark as available, no further action */
            closed++;
        }
    }
    return closed; /* return how many we did */
}
#endif /* #ifdef HAVE_HOTSWAP */
