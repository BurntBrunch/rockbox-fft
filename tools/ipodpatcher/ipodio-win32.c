/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dave Chapman
 *
 * error(), lock_volume() and unlock_volume() functions and inspiration taken
 * from:
 *       RawDisk - Direct Disk Read/Write Access for NT/2000/XP
 *       Copyright (c) 2003 Jan Kiszka
 *       http://www.stud.uni-hannover.de/user/73174/RawDisk/
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __WIN32__
#include <windows.h>
#include <winioctl.h>
#endif

#include "ipodio.h"

static int lock_volume(HANDLE hDisk) 
{ 
  DWORD dummy;

  return DeviceIoControl(hDisk, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0,
			 &dummy, NULL); 
}

static int unlock_volume(HANDLE hDisk) 
{ 
  DWORD dummy;

  return DeviceIoControl(hDisk, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0,
			 &dummy, NULL); 
} 

void print_error(char* msg)
{
    char* pMsgBuf;

    printf(msg);
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pMsgBuf,
                  0, NULL);
    printf(pMsgBuf);
    LocalFree(pMsgBuf);
}

int ipod_open(HANDLE* dh, char* diskname, int* sector_size, int silent)
{
    DISK_GEOMETRY_EX diskgeometry_ex;
    DISK_GEOMETRY diskgeometry;
    unsigned long n;

    *dh = CreateFile(diskname, GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);

    if (*dh == INVALID_HANDLE_VALUE) {
        if (!silent) print_error(" Error opening disk: ");
        return -1;
    }

    if (!lock_volume(*dh)) {
        if (!silent) print_error(" Error locking disk: ");
        return -1;
    }

    if (!DeviceIoControl(*dh,
                         IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                         NULL,
                         0,
                         &diskgeometry_ex,
                         sizeof(diskgeometry_ex),
                         &n,
                         NULL)) {
        if (!DeviceIoControl(*dh,
                             IOCTL_DISK_GET_DRIVE_GEOMETRY,
                             NULL,
                             0,
                             &diskgeometry,
                             sizeof(diskgeometry),
                             &n,
                             NULL)) {
            if (!silent) print_error(" Error reading disk geometry: ");
            return -1;
        } else {
            *sector_size=diskgeometry.BytesPerSector;
        }
    } else {
        *sector_size=diskgeometry_ex.Geometry.BytesPerSector;
    }

    return 0;
}

int ipod_reopen_rw(HANDLE* dh, char* diskname)
{
    /* Close existing file and re-open for writing */
    unlock_volume(*dh);
    CloseHandle(*dh);

    *dh = CreateFile(diskname, GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                     FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);

    if (*dh == INVALID_HANDLE_VALUE) {
        print_error(" Error opening disk: ");
        return -1;
    }

    if (!lock_volume(*dh)) {
        print_error(" Error locking disk: ");
        return -1;
    }

    return 0;
}

int ipod_close(HANDLE dh)
{
    unlock_volume(dh);
    CloseHandle(dh);
    return 0;
}

int ipod_alloc_buffer(unsigned char** sectorbuf, int bufsize)
{
    /* The ReadFile function requires a memory buffer aligned to a multiple of
       the disk sector size. */
    *sectorbuf = (unsigned char*)VirtualAlloc(NULL, bufsize, MEM_COMMIT, PAGE_READWRITE);
    if (*sectorbuf == NULL) {
        print_error(" Error allocating a buffer: ");
        return -1;
    }
    return 0;
}

int ipod_seek(HANDLE dh, unsigned long pos)
{
    if (SetFilePointer(dh, pos, NULL, FILE_BEGIN)==0xffffffff) {
        print_error(" Seek error ");
        return -1;
    }
    return 0;
}

int ipod_read(HANDLE dh, unsigned char* buf, int nbytes)
{
    unsigned long count;

    if (!ReadFile(dh, buf, nbytes, &count, NULL)) {
        print_error(" Error reading from disk: ");
        return -1;
    }

    return count;
}

int ipod_write(HANDLE dh, unsigned char* buf, int nbytes)
{
    unsigned long count;

    if (!WriteFile(dh, buf, nbytes, &count, NULL)) {
        print_error(" Error writing to disk: ");
        return -1;
    }

    return count;
}
