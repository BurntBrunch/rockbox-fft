/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Dave Chapman
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "parttypes.h"
#include "ipodio.h"
#include "ipodpatcher.h"

#ifdef WITH_BOOTOBJS
#include "ipod1g2g.h"
#include "ipod3g.h"
#include "ipod4g.h"
#include "ipodmini1g.h"
#include "ipodmini2g.h"
#include "ipodcolor.h"
#include "ipodnano1g.h"
#include "ipodvideo.h"
#include "ipodnano2g.h"
#endif

#ifndef RBUTIL
#include "arc4.h"
#endif

int ipod_verbose = 0;

unsigned char* ipod_sectorbuf = NULL;

/* The following string appears at the start of the firmware partition */
static const char *apple_stop_sign = "{{~~  /-----\\   "\
                                     "{{~~ /       \\  "\
                                     "{{~~|         | "\
                                     "{{~~| S T O P | "\
                                     "{{~~|         | "\
                                     "{{~~ \\       /  "\
                                     "{{~~  \\-----/   "\
                                     "Copyright(C) 200"\
                                     "1 Apple Computer"\
                                     ", Inc.----------"\
                                     "----------------"\
                                     "----------------"\
                                     "----------------"\
                                     "----------------"\
                                     "----------------"\
                                     "---------------";

/* Windows requires the buffer for disk I/O to be aligned in memory on a 
   multiple of the disk volume size - so we use a single global variable
   and initialise it with ipod_alloc_buf() 
*/

char* get_parttype(int pt)
{
    int i;
    static char unknown[]="Unknown";

    if (pt == PARTTYPE_HFS) {
        return "HFS/HFS+";
    }

    i=0;
    while (parttypes[i].name != NULL) {
        if (parttypes[i].type == pt) {
            return (parttypes[i].name);
        }
        i++;
    }

    return unknown;
}

off_t filesize(int fd) {
    struct stat buf;

    if (fstat(fd,&buf) < 0) {
        perror("[ERR]  Checking filesize of input file");
        return -1;
    } else {
        return(buf.st_size);
    }
}

/* Partition table parsing code taken from Rockbox */

#define MAX_SECTOR_SIZE 2048
#define SECTOR_SIZE 512

static inline unsigned short le2ushort(unsigned char* buf)
{
   unsigned short res = (buf[1] << 8) | buf[0];

   return res;
}

static inline int le2int(unsigned char* buf)
{
   int32_t res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

   return res;
}

static inline int be2int(unsigned char* buf)
{
   int32_t res = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

   return res;
}

static inline int getint16le(char* buf)
{
   int16_t res = (buf[1] << 8) | buf[0];

   return res;
}

static inline void short2le(unsigned short val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
}

static inline void int2le(unsigned int val, unsigned char* addr)
{
    addr[0] = val & 0xFF;
    addr[1] = (val >> 8) & 0xff;
    addr[2] = (val >> 16) & 0xff;
    addr[3] = (val >> 24) & 0xff;
}

static inline void int2be(unsigned int val, unsigned char* addr)
{
    addr[0] = (val >> 24) & 0xff;
    addr[1] = (val >> 16) & 0xff;
    addr[2] = (val >> 8) & 0xff;
    addr[3] = val & 0xFF;
}


#define BYTES2INT32(array,pos)\
    ((long)array[pos] | ((long)array[pos+1] << 8 ) |\
    ((long)array[pos+2] << 16 ) | ((long)array[pos+3] << 24 ))

int read_partinfo(struct ipod_t* ipod, int silent)
{
    int i;
    unsigned long count;

    count = ipod_read(ipod,ipod_sectorbuf, ipod->sector_size);

    if (count <= 0) {
        ipod_print_error(" Error reading from disk: ");
        return -1;
    }

    memset(ipod->pinfo, 0, sizeof(ipod->pinfo));

    if ((ipod_sectorbuf[510] == 0x55) && (ipod_sectorbuf[511] == 0xaa)) {
        /* DOS partition table */
        ipod->macpod = 0;
        /* parse partitions */
        for ( i = 0; i < 4; i++ ) {
            unsigned char* ptr = ipod_sectorbuf + 0x1be + 16*i;
            ipod->pinfo[i].type  = ptr[4];
            ipod->pinfo[i].start = BYTES2INT32(ptr, 8);
            ipod->pinfo[i].size  = BYTES2INT32(ptr, 12);

            /* extended? */
            if ( ipod->pinfo[i].type == 5 ) {
                /* not handled yet */
            }
        }
    } else if ((ipod_sectorbuf[0] == 'E') && (ipod_sectorbuf[1] == 'R')) {
        /* Apple Partition Map */

        /* APM parsing code based on the check_mac_partitions() function in
           ipodloader2 - written by Thomas Tempelmann and released
           under the GPL. */

        int blkNo = 1;
        int partBlkCount = 1;
        int partBlkSizMul = ipod_sectorbuf[2] / 2;

        int pmMapBlkCnt;      /* # of blks in partition map */
        int pmPyPartStart;    /* physical start blk of partition */
        int pmPartBlkCnt;     /* # of blks in this partition */
        int i = 0;

        ipod->macpod = 1;

        memset(ipod->pinfo,0,sizeof(ipod->pinfo));

        while (blkNo <= partBlkCount) {
            if (ipod_seek(ipod, blkNo * partBlkSizMul * 512) < 0) {
                fprintf(stderr,"[ERR]  Seek failed whilst reading APM\n");
                return -1;
            }

            count = ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);

            if (count <= 0) {
                ipod_print_error(" Error reading from disk: ");
                return -1;
            }

            /* see if it's a partition entry */
            if ((ipod_sectorbuf[0] != 'P') || (ipod_sectorbuf[1] != 'M')) {
                /* end of partition table -> leave the loop */
                break;  
            }

            /* Extract the interesting entries */
            pmMapBlkCnt = be2int(ipod_sectorbuf + 4);
            pmPyPartStart = be2int(ipod_sectorbuf + 8);
            pmPartBlkCnt = be2int(ipod_sectorbuf + 12);

            /* update the number of part map blocks */
            partBlkCount = pmMapBlkCnt;

            if (strncmp((char*)(ipod_sectorbuf + 48), "Apple_MDFW", 32)==0) {
                 /* A Firmware partition */
                 ipod->pinfo[i].start = pmPyPartStart;
                 ipod->pinfo[i].size = pmPartBlkCnt;
                 ipod->pinfo[i].type = 0;
                 i++;
            } else if (strncmp((char*)(ipod_sectorbuf + 48), "Apple_HFS", 32)==0) {
                 /* A HFS partition */
                 ipod->pinfo[i].start = pmPyPartStart;
                 ipod->pinfo[i].size = pmPartBlkCnt;
                 ipod->pinfo[i].type = PARTTYPE_HFS;
                 i++;
            }

            blkNo++; /* read next partition map entry */
        }
    } else {
        if (!silent) fprintf(stderr,"[ERR]  Bad boot sector signature\n");
        return -1;
    }

    /* Check that the partition table looks like an ipod:
           1) Partition 1 is of type 0 (Empty) but isn't empty.
           2) Partition 2 is of type 0xb or 0xc (winpod) or -1 (macpod)
    */
    if ((ipod->pinfo[0].type != 0) || (ipod->pinfo[0].size == 0) || 
        ((ipod->pinfo[1].type != 0xb) && (ipod->pinfo[1].type != 0xc) && 
         (ipod->pinfo[1].type != PARTTYPE_HFS))) {
        if (!silent) fprintf(stderr,"[ERR]  Partition layout is not an ipod\n");
        return -1;
    }

    ipod->start = ipod->pinfo[0].start*ipod->sector_size;
    return 0;
}

int read_partition(struct ipod_t* ipod, int outfile)
{
    int res;
    ssize_t n;
    int bytesleft;
    int chunksize;
    int count = ipod->pinfo[0].size;

    if (ipod_seek(ipod, ipod->start) < 0) {
        return -1;
    }

    fprintf(stderr,"[INFO] Writing %d sectors to output file\n",count);

    bytesleft = count * ipod->sector_size;
    while (bytesleft > 0) {
        if (bytesleft > BUFFER_SIZE) {
           chunksize = BUFFER_SIZE;
        } else {
           chunksize = bytesleft;
        }

        n = ipod_read(ipod, ipod_sectorbuf, chunksize);

        if (n < 0) {
            return -1;
        }

        if (n < chunksize) {
            fprintf(stderr,
              "[ERR]  Short read in disk_read() - requested %d, got %d\n",
              chunksize,(int)n);
            return -1;
        }

        bytesleft -= n;

        res = write(outfile,ipod_sectorbuf,n);

        if (res < 0) {
            perror("[ERR]  write in disk_read");
            return -1;
        }

        if (res != n) {
            fprintf(stderr,
              "Short write - requested %d, received %d - aborting.\n",(int)n,res);
            return -1;
        }
    }

    fprintf(stderr,"[INFO] Done.\n");
    return 0;
}

int write_partition(struct ipod_t* ipod, int infile)
{
    ssize_t res;
    int n;
    int bytesread;
    int byteswritten = 0;
    int eof;
    int padding = 0;

    if (ipod_seek(ipod, ipod->start) < 0) {
        return -1;
    }

    fprintf(stderr,"[INFO] Writing input file to device\n");
    bytesread = 0;
    eof = 0;
    while (!eof) {
        n = read(infile,ipod_sectorbuf,BUFFER_SIZE);

        if (n < 0) {
            perror("[ERR]  read in disk_write");
            return -1;
        }

        if (n < BUFFER_SIZE) {
           eof = 1;
           /* We need to pad the last write to a multiple of SECTOR_SIZE */
           if ((n % ipod->sector_size) != 0) {
               padding = (ipod->sector_size-(n % ipod->sector_size));
               n += padding; 
           }
        }

        bytesread += n;

        res = ipod_write(ipod, ipod_sectorbuf, n);

        if (res < 0) {
            ipod_print_error(" Error writing to disk: ");
            fprintf(stderr,"Bytes written: %d\n",byteswritten);
            return -1;
        }

        if (res != n) {
            fprintf(stderr,"[ERR]  Short write - requested %d, received %d - aborting.\n",n,(int)res);
            return -1;
        }

        byteswritten += res;
    }

    fprintf(stderr,"[INFO] Wrote %d bytes plus %d bytes padding.\n",
            byteswritten-padding,padding);
    return 0;
}

char* ftypename[] = { "OSOS", "RSRC", "AUPD", "HIBE", "OSBK" };

int diskmove(struct ipod_t* ipod, int delta)
{
    int src_start;
    int src_end;
    int bytesleft;
    int chunksize;
    int n;

    src_start = ipod->ipod_directory[1].devOffset;
    src_end = (ipod->ipod_directory[ipod->nimages-1].devOffset + ipod->sector_size + 
              ipod->ipod_directory[ipod->nimages-1].len + 
              (ipod->sector_size-1)) & ~(ipod->sector_size-1);
    bytesleft = src_end - src_start;

    if (ipod_verbose) {
        fprintf(stderr,"[INFO] Need to move images 2-%d forward %08x bytes\n", ipod->nimages,delta);
        fprintf(stderr,"[VERB] src_start     = %08x\n",src_start);
        fprintf(stderr,"[VERB] src_end       = %08x\n",src_end);
        fprintf(stderr,"[VERB] dest_start    = %08x\n",src_start+delta);
        fprintf(stderr,"[VERB] dest_end      = %08x\n",src_end+delta);
        fprintf(stderr,"[VERB] bytes to copy = %08x\n",bytesleft);
    }

    while (bytesleft > 0) {
        if (bytesleft <= BUFFER_SIZE) {
            chunksize = bytesleft;
        } else {
            chunksize = BUFFER_SIZE;
        }

        if (ipod_verbose) {
            fprintf(stderr,"[VERB] Copying %08x bytes from %08x to %08x (absolute %08x to %08x)\n",
                           chunksize,
                           src_end-chunksize,
                           src_end-chunksize+delta,
                           (unsigned int)(ipod->start+src_end-chunksize),
                           (unsigned int)(ipod->start+src_end-chunksize+delta));
        }


        if (ipod_seek(ipod, ipod->start+src_end-chunksize) < 0) {
            fprintf(stderr,"[ERR]  Seek failed\n");
            return -1;
        }

        if ((n = ipod_read(ipod,ipod_sectorbuf,chunksize)) < 0) {
            perror("[ERR]  Write failed\n");
            return -1;
        }

        if (n < chunksize) {
            fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                           chunksize,n);
            return -1;
        }

        if (ipod_seek(ipod, ipod->start+src_end-chunksize+delta) < 0) {
            fprintf(stderr,"[ERR]  Seek failed\n");
            return -1;
        }

        if ((n = ipod_write(ipod,ipod_sectorbuf,chunksize)) < 0) {
            perror("[ERR]  Write failed\n");
            return -1;
        }

        if (n < chunksize) {
            fprintf(stderr,"[ERR]  Short write - requested %d bytes, received %d\n"
                          ,chunksize,n);
            return -1;
        }

        src_end -= chunksize;
        bytesleft -= chunksize;
    }

    return 0;
}

static int rename_image(struct ipod_t* ipod, char* from, char* to)
{
    int n;
    int x;
    int found;
    int i;
    unsigned char* p;

    /* diroffset may not be sector-aligned */
    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek to diroffset (%08x) failed.\n",(unsigned)ipod->diroffset);
        return -1;
    }

    n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  Read of directory failed.\n");
        return -1;
    }

    p = ipod_sectorbuf + x;

    /* A hack to detect 2nd gen Nanos - maybe there is a better way? */
    if (p[0] == 0)
    {
        /* Adjust diroffset */
        ipod->diroffset += ipod->sector_size - x;

        n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
        if (n < 0) { 
            fprintf(stderr,"[ERR]  Read of directory failed.\n");
            return -1;
        }
        p = ipod_sectorbuf;
    }

    found = 0;
    for (i=0 ; !found && i < MAX_IMAGES; i++) {
       if (memcmp(p + 4, from, 4) == 0) {
           memcpy(p + 4, to, 4);

           found = 1;
       }
       p += 40;
    }

    if (!found) {
        fprintf(stderr,"[ERR]  Unexpected error - no \"%s\" image!\n", from);
        return -1;
    }

    /* Write directory back to disk */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek to diroffset (%08x) failed.\n",(unsigned)ipod->diroffset);
        return -1;
    }

    n=ipod_write(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  Write of directory failed in rename_image.\n");
        return -1;
    }

    return 0;
}

static int delete_image(struct ipod_t* ipod, char* name)
{
    int n;
    int x;
    int found;
    int i;
    unsigned char* p;

    /* diroffset may not be sector-aligned */
    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek to diroffset (%08x) failed.\n",(unsigned)ipod->diroffset);
        return -1;
    }

    n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  Read of directory failed.\n");
        return -1;
    }

    p = ipod_sectorbuf + x;

    /* A hack to detect 2nd gen Nanos - maybe there is a better way? */
    if (p[0] == 0)
    {
        /* Adjust diroffset */
        ipod->diroffset += ipod->sector_size - x;

        n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
        if (n < 0) { 
            fprintf(stderr,"[ERR]  Read of directory failed.\n");
            return -1;
        }
        p = ipod_sectorbuf;
    }

    found = 0;
    for (i=0 ; !found && i < MAX_IMAGES; i++) {
       if (memcmp(p + 4, name, 4) == 0) {
           memset(p, 0, 40);  /* Delete directory entry */
           found = 1;
       }
       p += 40;
    }

    if (!found) {
        fprintf(stderr,"[ERR]  Unexpected error - no \"%s\" image!\n", name);
        return -1;
    }

    /* Write directory back to disk */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek to diroffset (%08x) failed.\n",(unsigned)ipod->diroffset);
        return -1;
    }

    n=ipod_write(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  Write of directory failed in delete_image.\n");
        return -1;
    }

    return 0;
}

int add_new_image(struct ipod_t* ipod, char* imagename, char* filename, int type)
{
    int length;
    int found;
    int i;
    int x;
    int n;
    int infile;
    int newsize;
    unsigned long chksum=0;
    unsigned long filechksum=0;
    unsigned long offset;
    unsigned char header[8];  /* Header for .ipod file */
    unsigned char* p;

#ifdef WITH_BOOTOBJS
    if (type == FILETYPE_INTERNAL) {
        fprintf(stderr,"[INFO] Using internal bootloader - %d bytes\n",ipod->bootloader_len);
        length = ipod->bootloader_len;
        infile = -1;
    } 
    else 
#endif
    {
        /* First check that the input file is the correct type for this ipod. */
        infile=open(filename,O_RDONLY);
        if (infile < 0) {
            fprintf(stderr,"[ERR]  Couldn't open input file %s\n",filename);
            return -1;
        }
    
        if (type==FILETYPE_DOT_IPOD) {
            n = read(infile,header,8);
            if (n < 8) {
                fprintf(stderr,"[ERR]  Failed to read header from %s\n",filename);
                close(infile);
                return -1;
            }
    
            if (memcmp(header+4, ipod->modelname,4)!=0) {
                fprintf(stderr,"[ERR]  Model name in input file (%c%c%c%c) doesn't match ipod model (%s)\n",
                        header[4],header[5],header[6],header[7], ipod->modelname);
                close(infile);
                return -1;
            }
    
            filechksum = be2int(header);
    
            length = filesize(infile)-8;
        } else {
            length = filesize(infile);
        }
    }

    newsize=(length+ipod->sector_size-1)&~(ipod->sector_size-1);

    fprintf(stderr,"[INFO] Padding input file from 0x%08x to 0x%08x bytes\n",
            length,newsize);

    if (newsize > BUFFER_SIZE) {
        fprintf(stderr,"[ERR]  Input file too big for buffer\n");
        if (infile >= 0) close(infile);
        return -1;
    }

    /* TODO: Check if we have enough space in the partition for the new image */

#ifdef WITH_BOOTOBJS
    if (type == FILETYPE_INTERNAL) {
        memcpy(ipod_sectorbuf,ipod->bootloader,ipod->bootloader_len);
    } 
    else
#endif
    {
        fprintf(stderr,"[INFO] Reading input file...\n");

        n = read(infile,ipod_sectorbuf,length);
        if (n < 0) {
            fprintf(stderr,"[ERR]  Couldn't read input file\n");
            close(infile);
            return -1;
        }
        close(infile);
    }

    /* Pad the data with zeros */
    memset(ipod_sectorbuf+length,0,newsize-length);

    if (type==FILETYPE_DOT_IPOD) {
        chksum = ipod->modelnum;
        for (i = 0; i < length; i++) {
            /* add 8 unsigned bits but keep a 32 bit sum */
            chksum += ipod_sectorbuf[i];
        }

        if (chksum == filechksum) {
            fprintf(stderr,"[INFO] Checksum OK in %s\n",filename);
        } else {
            fprintf(stderr,"[ERR]  Checksum in %s failed check\n",filename);
            return -1;
        }
    }


    offset = ipod->fwoffset + ipod->ipod_directory[ipod->nimages - 1].devOffset +
             ipod->ipod_directory[ipod->nimages - 1].len + ipod->sector_size;

    /* 2nd Gen Nano has encrypted firmware, and the sector
       preceeding the firmware contains hashes that need to be
       preserved.  Nano 2G images include these extra 2048 (0x800)
       bytes 
     */
    if (ipod_seek(ipod, offset - (ipod->modelnum == 62 ? 0x800 : 0)) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_write(ipod,ipod_sectorbuf,newsize)) < 0) {
        perror("[ERR]  Write failed\n");
        return -1;
    }

    if (n < newsize) {
        fprintf(stderr,"[ERR]  Short write - requested %d bytes, received %d\n"
                      ,newsize,n);
        return -1;
    }
    fprintf(stderr,"[INFO]  Wrote %d bytes to firmware partition\n",n);

    /* Now we need to create a new directory entry 

       NOTE: On the Nano 2G, the checksum is the checksum of the
             unencrypted firmware.  But this isn't checked by the NOR
             bootloader (there are cryptographic hashes in the
             firmware itself), so it doesn't matter that this is
             wrong.
      */
    chksum = 0;
    for (i = 0; i < length; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         chksum += ipod_sectorbuf[i];
    }

    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }

    n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    /* Create a new directory entry */

    /* Copy OSOS or OSBK details - we assume one of them exists */
    p = ipod_sectorbuf + x;
    found = 0;
    for (i = 0; !found && i < ipod->nimages; i++) {
        if ((memcmp(p + 4, "soso", 4)==0) || (memcmp(p + 4, "kbso", 4)==0)) {
            found = 1;
        } else {
            p += 40;
        }
    }

    if (!found) {
        fprintf(stderr,"[ERR]  No OSOS or OSBK image to copy directory from\n");
        return -1;
    }

    /* Copy directory image */
    memcpy(ipod_sectorbuf + x + (ipod->nimages * 40), p, 40);
    p = ipod_sectorbuf + x + (ipod->nimages * 40);

    /* Modify directory. */
    memcpy(p + 4, imagename, 4);
    int2le(offset - ipod->fwoffset, p + 12);                     /* devOffset */
    int2le(length - (ipod->modelnum==62 ? 0x800: 0), p + 16);    /* len */
    int2le(chksum, p + 28);                                      /* checksum */

    /* Write directory */    
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }
    n=ipod_write(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    return 0;
}


/*
    Bootloader installation on the Nano2G consists of renaming the
    OSOS image to OSBK and then writing the Rockbox bootloader as a
    new OSOS image.

    Maybe this approach can/should be adapted for other ipods, as it
    prevents the Apple bootloader loading the original firmware into
    RAM along with the Rockbox bootloader (and hence will give a
    faster boot when the user just wants to start Rockbox).

*/

static int add_bootloader_nano2g(struct ipod_t* ipod, char* filename, int type)
{
    int i;
    int has_osbk = 0;

    /* Check if we already have an OSBK image */
    for (i = 0; i < ipod->nimages; i++) {
        if (ipod->ipod_directory[i].ftype==FTYPE_OSBK) {
            has_osbk = 1;
        }
    }

    if (has_osbk == 0) {
        /* First-time install - rename OSOS to OSBK and create new OSOS for bootloader */
        fprintf(stderr,"[INFO] Creating OSBK backup image of original firmware\n");

        if  (rename_image(ipod, "soso", "kbso") < 0) {
            fprintf(stderr,"[ERR]  Could not rename OSOS image\n");
            return -1;
        }

        /* Add our bootloader as a brand new image */
        return add_new_image(ipod, "soso", filename, type);
    } else {
        /* This is an update, just replace OSOS with our bootloader */

        return write_firmware(ipod, filename, type);
    }
}


static int delete_bootloader_nano2g(struct ipod_t* ipod)
{
    int i;
    int has_osbk = 0;

    /* Check if we have an OSBK image */
    for (i = 0; i < ipod->nimages; i++) {
        if (ipod->ipod_directory[i].ftype==FTYPE_OSBK) {
            has_osbk = 1;
        }
    }

    if (has_osbk == 0) {
        fprintf(stderr,"[ERR]  No OSBK image found - nothing to uninstall\n");
        return -1;
    } else {
        /* Delete our bootloader image */
        if (delete_image(ipod, "soso") < 0) {
            fprintf(stderr,"[WARN] Could not delete OSOS image\n");
        } else {
            fprintf(stderr,"[INFO] OSOS image deleted\n");
        }

        if  (rename_image(ipod, "kbso", "soso") < 0) {
            fprintf(stderr,"[ERR]  Could not rename OSBK image\n");
            return -1;
        }


        fprintf(stderr,"[INFO] OSBK image renamed to OSOS - bootloader uninstalled.\n");
        return 0;
    }
}


int add_bootloader(struct ipod_t* ipod, char* filename, int type)
{
    int length;
    int i;
    int x;
    int n;
    int infile;
    int paddedlength;
    int entryOffset;
    int delta = 0;
    unsigned long chksum=0;
    unsigned long filechksum=0;
    unsigned char header[8];  /* Header for .ipod file */
    unsigned char* bootloader_buf;

    /* The 2nd gen Nano is installed differently */
    if (ipod->modelnum == 62) {
        return add_bootloader_nano2g(ipod, filename, type);
    }

    /* Calculate the position in the OSOS image where our bootloader will go. */
    if (ipod->ipod_directory[0].entryOffset>0) {
        /* Keep the same entryOffset */
        entryOffset = ipod->ipod_directory[0].entryOffset;
    } else {
        entryOffset = (ipod->ipod_directory[0].len+ipod->sector_size-1)&~(ipod->sector_size-1);
    }

#ifdef WITH_BOOTOBJS
    if (type == FILETYPE_INTERNAL) {
        fprintf(stderr,"[INFO] Using internal bootloader - %d bytes\n",ipod->bootloader_len);
        memcpy(ipod_sectorbuf+entryOffset,ipod->bootloader,ipod->bootloader_len);
        length = ipod->bootloader_len;
        paddedlength=(ipod->bootloader_len+ipod->sector_size-1)&~(ipod->sector_size-1);
    } 
    else 
#endif
    {
        infile=open(filename,O_RDONLY);
        if (infile < 0) {
            fprintf(stderr,"[ERR]  Couldn't open input file %s\n",filename);
            return -1;
        }

        if (type==FILETYPE_DOT_IPOD) {
            /* First check that the input file is the correct type for this ipod. */
            n = read(infile,header,8);
            if (n < 8) {
                fprintf(stderr,"[ERR]  Failed to read header from %s\n",filename);
                close(infile);
                return -1;
            }
    
            if (memcmp(header+4, ipod->modelname,4)!=0) {
                fprintf(stderr,"[ERR]  Model name in input file (%c%c%c%c) doesn't match ipod model (%s)\n",
                        header[4],header[5],header[6],header[7], ipod->modelname);
                close(infile);
                return -1;
            }
    
            filechksum = be2int(header);
    
            length=filesize(infile)-8;
        } else {
            length=filesize(infile);
        }
        paddedlength=(length+ipod->sector_size-1)&~(ipod->sector_size-1);

        bootloader_buf = malloc(length);
        if (bootloader_buf == NULL) {
            fprintf(stderr,"[ERR]  Can not allocate memory for bootloader\n");
            return -1;
        }
        /* Now read our bootloader - we need to check it before modifying the partition*/
        n = read(infile,bootloader_buf,length);
        close(infile);

        if (n < 0) {
            fprintf(stderr,"[ERR]  Couldn't read input file\n");
            return -1;
        }
    
        if (type==FILETYPE_DOT_IPOD) {
            /* Calculate and confirm bootloader checksum */
            chksum = ipod->modelnum;
            for (i = 0; i < length; i++) {
                 /* add 8 unsigned bits but keep a 32 bit sum */
                 chksum += bootloader_buf[i];
            }
    
            if (chksum == filechksum) {
                fprintf(stderr,"[INFO] Checksum OK in %s\n",filename);
            } else {
                fprintf(stderr,"[ERR]  Checksum in %s failed check\n",filename);
                return -1;
            }
        }
    }

    if (entryOffset+paddedlength > BUFFER_SIZE) {
        fprintf(stderr,"[ERR]  Input file too big for buffer\n");
        return -1;
    }

    if (ipod_verbose) {
        fprintf(stderr,"[VERB] Original firmware begins at 0x%08x\n", ipod->ipod_directory[0].devOffset + ipod->sector_size);
        fprintf(stderr,"[VERB] New entryOffset will be 0x%08x\n",entryOffset);
        fprintf(stderr,"[VERB] End of bootloader will be at 0x%08x\n",entryOffset+paddedlength);
    }

    /* Check if we have enough space */
    /* TODO: Check the size of the partition. */
    if (ipod->nimages > 1) {
        if ((ipod->ipod_directory[0].devOffset+entryOffset+paddedlength) >
             ipod->ipod_directory[1].devOffset) {
            fprintf(stderr,"[INFO] Moving images to create room for new firmware...\n");
            delta = ipod->ipod_directory[0].devOffset + entryOffset+paddedlength
                    - ipod->ipod_directory[1].devOffset + ipod->sector_size;

            if (diskmove(ipod, delta) < 0) {
                fprintf(stderr,"[ERR]  Image movement failed.\n");
                return -1;
            }
        }
    }


    /* We have moved the partitions, now we can write our bootloader */

    /* Firstly read the original firmware into ipod_sectorbuf */
    fprintf(stderr,"[INFO] Reading original firmware...\n");
    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[0].devOffset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_read(ipod,ipod_sectorbuf,entryOffset)) < 0) {
        perror("[ERR]  Read failed\n");
        return -1;
    }

    if (n < entryOffset) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n"
                      ,entryOffset,n);
        return -1;
    }

#ifdef WITH_BOOTOBJS
    if (type == FILETYPE_INTERNAL) {
        memcpy(ipod_sectorbuf+entryOffset,ipod->bootloader,ipod->bootloader_len);
    }
    else
#endif
    {
        memcpy(ipod_sectorbuf+entryOffset,bootloader_buf,length);
        free(bootloader_buf);
    }

    /* Calculate new checksum for combined image */
    chksum = 0;
    for (i=0;i<entryOffset + length; i++) {
         chksum += ipod_sectorbuf[i];
    }    

    /* Now write the combined firmware image to the disk */

    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[0].devOffset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_write(ipod,ipod_sectorbuf,entryOffset+paddedlength)) < 0) {
        perror("[ERR]  Write failed\n");
        return -1;
    }

    if (n < (entryOffset+paddedlength)) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n"
                      ,entryOffset+paddedlength,n);
        return -1;
    }

    fprintf(stderr,"[INFO]  Wrote %d bytes to firmware partition\n",entryOffset+paddedlength);

    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) {
        fprintf(stderr,"[ERR]  Directory read failed\n");
        return -1;
    }

    /* Update entries for image 0 */
    int2le(entryOffset+length,ipod_sectorbuf+x+16);
    int2le(entryOffset,ipod_sectorbuf+x+24);
    int2le(chksum,ipod_sectorbuf+x+28);
    int2le(0xffffffff,ipod_sectorbuf+x+36);  /* loadAddr */

    /* Update devOffset entries for other images, if we have moved them */
    if (delta > 0) {
        for (i=1;i<ipod->nimages;i++) {
           int2le(le2int(ipod_sectorbuf+x+i*40+12)+delta,ipod_sectorbuf+x+i*40+12);
        }
    }

    /* Write directory */    
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek to %d failed\n", (int)(ipod->start+ipod->diroffset-x));
        return -1;
    }
    n=ipod_write(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  Directory write failed\n");
        return -1;
    }

    return 0;
}

int delete_bootloader(struct ipod_t* ipod)
{
    int length;
    int i;
    int x;
    int n;
    unsigned long chksum=0;   /* 32 bit checksum - Rockbox .ipod style*/

    /* The 2nd gen Nano is installed differently */
    if (ipod->modelnum == 62) {
        return delete_bootloader_nano2g(ipod);
    }

    /* Removing the bootloader involves adjusting the "length",
       "chksum" and "entryOffset" values in the osos image's directory
       entry. */

    /* Firstly check we have a bootloader... */

    if (ipod->ipod_directory[0].entryOffset == 0) {
        fprintf(stderr,"[ERR]  No bootloader found.\n");
        return -1;
    }

    length = ipod->ipod_directory[0].entryOffset;

    /* Read the firmware so we can calculate the checksum */
    fprintf(stderr,"[INFO] Reading firmware (%d bytes)\n",length);

    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[0].devOffset) < 0) {
        return -1;
    }

    i = (length+ipod->sector_size-1) & ~(ipod->sector_size-1);
    fprintf(stderr,"[INFO] Padding read from 0x%08x to 0x%08x bytes\n",
            length,i);

    if ((n = ipod_read(ipod,ipod_sectorbuf,i)) < 0) {
        return -1;
    }

    if (n < i) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                i,n);
        return -1;
    }

    chksum = 0;
    for (i = 0; i < length; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         chksum += ipod_sectorbuf[i];
    }

    /* Now write back the updated directory entry */

    fprintf(stderr,"[INFO] Updating firmware checksum\n");

    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }

    n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    /* Update entries for image 0 */
    int2le(length,ipod_sectorbuf+x+16);
    int2le(0,ipod_sectorbuf+x+24);
    int2le(chksum,ipod_sectorbuf+x+28);

    /* Write directory */    
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }
    n=ipod_write(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    return 0;
}

int write_firmware(struct ipod_t* ipod, char* filename, int type)
{
    int length;
    int i;
    int x;
    int n;
    int infile;
    int newsize;
    int bytesavailable;
    unsigned long chksum=0;
    unsigned long filechksum=0;
    unsigned long offset;
    unsigned char header[8];  /* Header for .ipod file */
    unsigned char* p;

#ifdef WITH_BOOTOBJS
    if (type == FILETYPE_INTERNAL) {
        fprintf(stderr,"[INFO] Using internal bootloader - %d bytes\n",ipod->bootloader_len);
        length = ipod->bootloader_len;
        infile = -1;
    } 
    else 
#endif
    {
        /* First check that the input file is the correct type for this ipod. */
        infile=open(filename,O_RDONLY);
        if (infile < 0) {
            fprintf(stderr,"[ERR]  Couldn't open input file %s\n",filename);
            return -1;
        }
    
        if (type==FILETYPE_DOT_IPOD) {
            n = read(infile,header,8);
            if (n < 8) {
                fprintf(stderr,"[ERR]  Failed to read header from %s\n",filename);
                close(infile);
                return -1;
            }
    
            if (memcmp(header+4, ipod->modelname,4)!=0) {
                fprintf(stderr,"[ERR]  Model name in input file (%c%c%c%c) doesn't match ipod model (%s)\n",
                        header[4],header[5],header[6],header[7], ipod->modelname);
                close(infile);
                return -1;
            }
    
            filechksum = be2int(header);
    
            length = filesize(infile)-8;
        } else {
            length = filesize(infile);
        }
    }

    newsize=(length+ipod->sector_size-1)&~(ipod->sector_size-1);

    fprintf(stderr,"[INFO] Padding input file from 0x%08x to 0x%08x bytes\n",
            length,newsize);

    if (newsize > BUFFER_SIZE) {
        fprintf(stderr,"[ERR]  Input file too big for buffer\n");
        if (infile >= 0) close(infile);
        return -1;
    }

    /* Check if we have enough space */
    /* TODO: Check the size of the partition. */
    if (ipod->nimages > 1) {
        bytesavailable=ipod->ipod_directory[1].devOffset-ipod->ipod_directory[0].devOffset;
        if (bytesavailable < newsize) {
            fprintf(stderr,"[INFO] Moving images to create room for new firmware...\n");

            /* TODO: Implement image movement */
            fprintf(stderr,"[ERR]  Image movement not yet implemented.\n");
            close(infile);
            return -1;
        }
    }

#ifdef WITH_BOOTOBJS
    if (type == FILETYPE_INTERNAL) {
        memcpy(ipod_sectorbuf,ipod->bootloader,ipod->bootloader_len);
    } 
    else
#endif
    {
        fprintf(stderr,"[INFO] Reading input file...\n");
        /* We now know we have enough space, so write it. */
        n = read(infile,ipod_sectorbuf,length);
        if (n < 0) {
            fprintf(stderr,"[ERR]  Couldn't read input file\n");
            close(infile);
            return -1;
        }
        close(infile);
    }

    /* Pad the data with zeros */
    memset(ipod_sectorbuf+length,0,newsize-length);

    if (type==FILETYPE_DOT_IPOD) {
        chksum = ipod->modelnum;
        for (i = 0; i < length; i++) {
            /* add 8 unsigned bits but keep a 32 bit sum */
            chksum += ipod_sectorbuf[i];
        }

        if (chksum == filechksum) {
            fprintf(stderr,"[INFO] Checksum OK in %s\n",filename);
        } else {
            fprintf(stderr,"[ERR]  Checksum in %s failed check\n",filename);
            return -1;
        }
    }


    offset = ipod->fwoffset+ipod->ipod_directory[ipod->ososimage].devOffset;

    if (ipod->modelnum==62) {

        /* 2nd Gen Nano has encrypted firmware, and the sector
           preceeding the firmware contains hashes that need to be
           preserved.  Nano 2G images include these extra 2048 (0x800)
           bytes 
        */

        offset -= 0x800;

       /* TODO: The above checks need to take into account this 0x800 bytes */
    }

    if (ipod_seek(ipod, offset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_write(ipod,ipod_sectorbuf,newsize)) < 0) {
        perror("[ERR]  Write failed\n");
        return -1;
    }

    if (n < newsize) {
        fprintf(stderr,"[ERR]  Short write - requested %d bytes, received %d\n"
                      ,newsize,n);
        return -1;
    }
    fprintf(stderr,"[INFO]  Wrote %d bytes to firmware partition\n",n);

    /* Now we need to update the "len", "entryOffset" and "chksum" fields

       NOTE: On the Nano 2G, the checksum is the checksum of the
             unencrypted firmware.  But this isn't checked by the NOR
             bootloader (there are cryptographic hashes in the
             firmware itself), so it doesn't matter that this is
             wrong.
      */
    chksum = 0;
    for (i = 0; i < length; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         chksum += ipod_sectorbuf[i];
    }

    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }

    n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    /* Update entries for image */
    p = ipod_sectorbuf + x + (ipod->ososimage * 40);    
    int2le(length - (ipod->modelnum==62 ? 0x800: 0), p + 16);
    int2le(0, p + 24);
    int2le(chksum, p + 28);

    /* Write directory */    
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }
    n=ipod_write(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    return 0;
}

int read_firmware(struct ipod_t* ipod, char* filename, int type)
{
    int length;
    int i;
    int outfile;
    int n;
    unsigned long offset;
    unsigned long chksum=0;   /* 32 bit checksum - Rockbox .ipod style*/
    unsigned char header[8];  /* Header for .ipod file */

    if (ipod->ipod_directory[ipod->ososimage].entryOffset != 0) {
        /* We have a bootloader... */
        length = ipod->ipod_directory[ipod->ososimage].entryOffset;
    } else {
        length = ipod->ipod_directory[ipod->ososimage].len;
    }

    fprintf(stderr,"[INFO] Reading firmware (%d bytes)\n",length);

    offset = ipod->fwoffset + ipod->ipod_directory[ipod->ososimage].devOffset;
    i = (length+ipod->sector_size-1) & ~(ipod->sector_size-1);
    fprintf(stderr,"[INFO] Padding read from 0x%08x to 0x%08x bytes\n",
            length,i);

    if (ipod->modelnum==62) {
        /* 2nd Gen Nano has encrypted firmware, and we need to dump the
           sector preceeding the image - it contains hashes */
        offset -= 0x800;
        length += 0x800;
        i += 0x800;
    }

    if (ipod_seek(ipod, offset)) {
        return -1;
    }

    if ((n = ipod_read(ipod,ipod_sectorbuf,i)) < 0) {
        return -1;
    }

    if (n < i) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                i,n);
        return -1;
    }

    outfile = open(filename,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666);
    if (outfile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open file %s\n",filename);
        return -1;
    }

    if (type == FILETYPE_DOT_IPOD) {
        chksum = ipod->modelnum;
        for (i = 0; i < length; i++) {
            /* add 8 unsigned bits but keep a 32 bit sum */
            chksum += ipod_sectorbuf[i];
        }

        int2be(chksum,header);
        memcpy(header+4, ipod->modelname,4);

        n = write(outfile,header,8);
        if (n != 8) {
            fprintf(stderr,"[ERR]  Write error - %d\n",n);
        }
    }

    n = write(outfile,ipod_sectorbuf,length);
    if (n != length) {
        fprintf(stderr,"[ERR]  Write error - %d\n",n);
    }
    close(outfile);

    return 0;
}

int read_directory(struct ipod_t* ipod)
{
    int n;
    int x;
    unsigned char* p;
    unsigned short version;

    ipod->nimages=0;

    /* Read firmware partition header (first 512 bytes of disk - but 
       let's read a whole sector) */

    if (ipod_seek(ipod, ipod->start) < 0) { 
        fprintf(stderr,"[ERR]  Seek to 0x%08x in read_directory() failed.\n",
                       (unsigned int)(ipod->start));
        return -1;
    }

    n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  ipod_read(ipod,buf,0x%08x) failed in read_directory()\n", ipod->sector_size);
        return -1;
    }

    if (memcmp(ipod_sectorbuf,apple_stop_sign,sizeof(apple_stop_sign))!=0) {
        fprintf(stderr,"[ERR]  Firmware partition doesn't contain Apple copyright, aborting.\n");
        return -1;
    }

    if (memcmp(ipod_sectorbuf+0x100,"]ih[",4)!=0) {
        fprintf(stderr,"[ERR]  Bad firmware directory\n");
        return -1;
    }

    version = le2ushort(ipod_sectorbuf+0x10a);
    if ((version != 2) && (version != 3)) {
        fprintf(stderr,"[ERR]  Unknown firmware format version %04x\n",
                version);
    }
    ipod->diroffset=le2int(ipod_sectorbuf+0x104) + 0x200;

    /* diroffset may not be sector-aligned */
    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { 
        fprintf(stderr,"[ERR]  Seek to diroffset (%08x) failed.\n",(unsigned)ipod->diroffset);
        return -1;
    }

    n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { 
        fprintf(stderr,"[ERR]  Read of directory failed.\n");
        return -1;
    }

    p = ipod_sectorbuf + x;

    /* A hack to detect 2nd gen Nanos - maybe there is a better way? */
    if (p[0] == 0)
    {
        /* Adjust diroffset */
        ipod->diroffset += ipod->sector_size - x;

        n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
        if (n < 0) { 
            fprintf(stderr,"[ERR]  Read of directory failed.\n");
            return -1;
        }
        p = ipod_sectorbuf;
    }

    ipod->ososimage = -1;
    while ((ipod->nimages < MAX_IMAGES) && (p < (ipod_sectorbuf + x + 400)) && 
           ((memcmp(p,"!ATA",4)==0) || (memcmp(p,"DNAN",4)==0))) {
        p+=4;
        if (memcmp(p,"soso",4)==0) {
            ipod->ipod_directory[ipod->nimages].ftype=FTYPE_OSOS;
            ipod->ososimage = ipod->nimages;
        } else if (memcmp(p,"crsr",4)==0) {
            ipod->ipod_directory[ipod->nimages].ftype=FTYPE_RSRC;
        } else if (memcmp(p,"dpua",4)==0) {
            ipod->ipod_directory[ipod->nimages].ftype=FTYPE_AUPD;
        } else if (memcmp(p,"kbso",4)==0) {
            ipod->ipod_directory[ipod->nimages].ftype=FTYPE_OSBK;
        } else if (memcmp(p,"ebih",4)==0) {
            ipod->ipod_directory[ipod->nimages].ftype=FTYPE_HIBE;
        } else {
            fprintf(stderr,"[ERR]  Unknown image type %c%c%c%c\n",
                           p[0],p[1],p[2],p[3]);
        }
        p+=4;
        ipod->ipod_directory[ipod->nimages].id=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].devOffset=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].len=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].addr=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].entryOffset=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].chksum=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].vers=le2int(p);
        p+=4;
        ipod->ipod_directory[ipod->nimages].loadAddr=le2int(p);
        p+=4;
        ipod->nimages++;
    }

    if (ipod->ososimage < 0) {
        fprintf(stderr,"[ERR]  No OSOS image found.\n");
        return -1;
    }

    if ((ipod->nimages > 1) && (version==2)) {
        /* The 3g firmware image doesn't appear to have a version, so
           let's make one up... Note that this is never written back to the
           ipod, so it's OK to do. */

        if (ipod->ipod_directory[ipod->ososimage].vers == 0) { ipod->ipod_directory[ipod->ososimage].vers = 3; }

        ipod->fwoffset = ipod->start;
    } else {
        ipod->fwoffset = ipod->start + ipod->sector_size;
    }

    return 0;
}

int list_images(struct ipod_t* ipod)
{
    int i;

    if (ipod_verbose) {
        printf("    Type         id  devOffset        len       addr entryOffset    chksum       vers   loadAddr   devOffset+len\n");
        for (i = 0 ; i < ipod->nimages; i++) {
            printf("%d - %s 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",i,
                   ftypename[ipod->ipod_directory[i].ftype],
                   ipod->ipod_directory[i].id,
                   ipod->ipod_directory[i].devOffset,
                   ipod->ipod_directory[i].len,
                   ipod->ipod_directory[i].addr,
                   ipod->ipod_directory[i].entryOffset,
                   ipod->ipod_directory[i].chksum,
                   ipod->ipod_directory[i].vers,
                   ipod->ipod_directory[i].loadAddr,
                   ipod->ipod_directory[i].devOffset+((ipod->ipod_directory[i].len+ipod->sector_size-1)&~(ipod->sector_size-1)));
        }
    }

    printf("\n");
    printf("Listing firmware partition contents:\n");
    printf("\n");

    for (i = 0 ; i < ipod->nimages; i++) {
         printf("Image %d:\n",i+1);
         switch(ipod->ipod_directory[i].ftype) {
             case FTYPE_OSOS:
                 if (ipod->ipod_directory[i].entryOffset==0) {
                     printf("    Main firmware - %d bytes\n",
                       ipod->ipod_directory[i].len);
                 } else {
                     printf("    Main firmware - %d bytes\n",
                       ipod->ipod_directory[i].entryOffset);
                     printf("    Third-party bootloader - %d bytes\n",
                       ipod->ipod_directory[i].len-ipod->ipod_directory[i].entryOffset);
                 }
                 break;
             default:
                     printf("    %s - %d bytes\n",
                            ftypename[ipod->ipod_directory[i].ftype],
                            ipod->ipod_directory[i].len);
         }
    }
    printf("\n");

    return 0;
}

int getmodel(struct ipod_t* ipod, int ipod_version)
{
    switch (ipod_version) {
        case 0x01:
            ipod->modelstr="1st or 2nd Generation";
            ipod->modelnum = 19;
            ipod->modelname = "1g2g";
            ipod->targetname = "ipod1g2g";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipod1g2g;
            ipod->bootloader_len = LEN_ipod1g2g;
#endif
            break;
        case 0x02:
            ipod->modelstr="3rd Generation";
            ipod->modelnum = 7;
            ipod->modelname = "ip3g";
            ipod->targetname = "ipod3g";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipod3g;
            ipod->bootloader_len = LEN_ipod3g;
#endif
            break;
        case 0x40:
            ipod->modelstr="1st Generation Mini";
            ipod->modelnum = 9;
            ipod->modelname = "mini";
            ipod->targetname = "ipodmini1g";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodmini;
            ipod->bootloader_len = LEN_ipodmini;
#endif
            break;
        case 0x50:
            ipod->modelstr="4th Generation";
            ipod->modelnum = 8;
            ipod->modelname = "ip4g";
            ipod->targetname = "ipod4gray";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipod4g;
            ipod->bootloader_len = LEN_ipod4g;
#endif
            break;
        case 0x60:
            ipod->modelstr="Photo/Color";
            ipod->modelnum = 3;
            ipod->modelname = "ipco";
            ipod->targetname = "ipodcolor";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodcolor;
            ipod->bootloader_len = LEN_ipodcolor;
#endif
            break;
        case 0x70:
            ipod->modelstr="2nd Generation Mini";
            ipod->modelnum = 11;
            ipod->modelname = "mn2g";
            ipod->targetname = "ipodmini2g";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodmini2g;
            ipod->bootloader_len = LEN_ipodmini2g;
#endif
            break;
        case 0xc0:
            ipod->modelstr="1st Generation Nano";
            ipod->modelnum = 4;
            ipod->modelname = "nano";
            ipod->targetname = "ipodnano1g";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodnano;
            ipod->bootloader_len = LEN_ipodnano;
#endif
            break;
        case 0xb0:
            ipod->modelstr="Video (aka 5th Generation)";
            ipod->modelnum = 5;
            ipod->modelname = "ipvd";
            if(ipod->ramsize == 64) {
                ipod->targetname = "ipodvideo64mb";
            }
            else {
                ipod->targetname = "ipodvideo";
            }
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodvideo;
            ipod->bootloader_len = LEN_ipodvideo;
#endif
            break;
        case 0x100:
            ipod->modelstr="2nd Generation Nano";
            ipod->modelnum = 62;
            ipod->modelname = "nn2x";
            ipod->targetname = "ipodnano2g";
#ifdef WITH_BOOTOBJS
            ipod->bootloader = ipodnano2g;
            ipod->bootloader_len = LEN_ipodnano2g;
#endif
            break;
        default:
            ipod->modelname = NULL;
            ipod->modelnum = 0;
            ipod->targetname = NULL;
#ifdef WITH_BOOTOBJS
            ipod->bootloader = NULL;
            ipod->bootloader_len = 0;
#endif
            return -1;
    }
    return 0;
}

/* returns number of found ipods or -1 if no ipods found and permission
 * for raw disc access was denied. */
int ipod_scan(struct ipod_t* ipod)
{
    int i;
    int n = 0;
    int ipod_version;
    struct ipod_t ipod_found;
    int denied = 0;
    int result;

    printf("[INFO] Scanning disk devices...\n");

    for (i = 0; i <= 25 ; i++) {
#ifdef __WIN32__
         sprintf(ipod->diskname,"\\\\.\\PhysicalDrive%d",i);
#elif defined(linux) || defined (__linux)
         sprintf(ipod->diskname,"/dev/sd%c",'a'+i);
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
      || defined(__bsdi__) || defined(__DragonFly__)
         sprintf(ipod->diskname,"/dev/da%d",i);
#elif defined(__APPLE__) && defined(__MACH__)
         sprintf(ipod->diskname,"/dev/disk%d",i);
#else
    #error No disk paths defined for this platform
#endif
         if ((result = ipod_open(ipod, 1)) < 0) {
             if(result == -2) {
                 denied++;
             }
             ipod_close(ipod);
             continue;
         }

         if (read_partinfo(ipod,1) < 0) {
             ipod_close(ipod);
             continue;
         }

         if ((ipod->pinfo[0].start==0) || (ipod->pinfo[0].type != 0)) {
             ipod_close(ipod);
             continue;
         }

         if (read_directory(ipod) < 0) {
             ipod_close(ipod);
             continue;
         }

         ipod_version=(ipod->ipod_directory[ipod->ososimage].vers>>8);
         ipod->ramsize = 0;
#ifdef __WIN32__
         /* Windows requires the ipod in R/W mode for SCSI Inquiry.
          * ipod_reopen_rw does unmount the player on OS X so do this on
          * W32 only during scanning. */
         ipod_reopen_rw(ipod);
#endif
         ipod_get_xmlinfo(ipod);
         ipod_get_ramsize(ipod);
         if (getmodel(ipod,ipod_version) < 0) {
             ipod_close(ipod);
             continue;
         }

#ifdef __WIN32__
         printf("[INFO] Ipod found - %s (\"%s\") - disk device %d\n", 
             ipod->modelstr,ipod->macpod ? "macpod" : "winpod",i);
#else
         printf("[INFO] Ipod found - %s (\"%s\") - %s\n",
             ipod->modelstr,ipod->macpod ? "macpod" : "winpod",ipod->diskname);
#endif
         n++;
         /* save the complete ipod_t structure for match. The for loop might
          * overwrite it, so we need to restore it later if only one found. */
         memcpy(&ipod_found, ipod, sizeof(struct ipod_t));
         ipod_close(ipod);
    }

    if (n==1) {
        /* restore the ipod_t structure, it might have been overwritten */
        memcpy(ipod, &ipod_found, sizeof(struct ipod_t));
    }
    else if(n == 0 && denied) {
        printf("[ERR]  FATAL: Permission denied on %d device(s) and no ipod detected.\n", denied);
#ifdef __WIN32__
        printf("[ERR]  You need to run this program with administrator priviledges!\n");
#else
        printf("[ERR]  You need permissions for raw disc access for this program to work!\n");
#endif
    }
    return (n == 0 && denied) ? -1 : n;
}

static void put_int32le(uint32_t x, unsigned char* p)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

int write_dos_partition_table(struct ipod_t* ipod)
{
    unsigned char* p;
    int i, n;
    uint32_t type;

    /* Only support 512-byte sectors at the moment */
    if ( ipod->sector_size != 512 )
    {
        fprintf(stderr,"[ERR]  Only ipods with 512 bytes per sector are supported.\n");
        return -1;
    }

    /* Firstly zero the entire MBR */
    memset(ipod_sectorbuf, 0, ipod->sector_size);

    /* Now add the partition info */
    for (i=0; i < 4 ; i++) 
    {
        p = ipod_sectorbuf + 0x1be + i*16;

        /* Ensure first partition is type 0, and second is 0xb */
        if (i==0) { type = 0; }
        else if (i==1) { type = 0xb; }
        else { type = ipod->pinfo[i].type; }

        put_int32le(type, p + 4);
        put_int32le(ipod->pinfo[i].start, p + 8);
        put_int32le(ipod->pinfo[i].size, p + 12);
    }

    /* Finally add the magic */
    ipod_sectorbuf[0x1fe] = 0x55;
    ipod_sectorbuf[0x1ff] = 0xaa;

    if (ipod_seek(ipod, 0) < 0) {
        fprintf(stderr,"[ERR]  Seek failed writing MBR\n");
        return -1;
    }

    /* Write MBR */
    if ((n = ipod_write(ipod, ipod_sectorbuf, ipod->sector_size)) < 0) {
        perror("[ERR]  Write failed\n");
        return -1;
    }

    return 0;
}

/* Get the XML Device Information, as documented here:

   http://www.ipodlinux.org/wiki/Device_Information
*/

int ipod_get_xmlinfo(struct ipod_t* ipod)
{
    unsigned char hdr[255];
    unsigned char buf[255];
    char* p;
    int psize;
    int npages;
    int i;

    if (ipod_scsi_inquiry(ipod, 0xc0, buf, sizeof(buf)) < 0)
    {
        fprintf(stderr,"[ERR]  Sending SCSI Command failed.\n");
        return -1;
    }

    /* Reading directly into hdr[] causes problems (for an unknown reason) on 
       win32 */
    memcpy(hdr, buf, sizeof(hdr));

    npages = hdr[3];

    psize = npages * 0xf8; /* Hopefully this is enough. */

    ipod->xmlinfo = malloc(psize); 
    ipod->xmlinfo_len = 0;

    if (ipod->xmlinfo == NULL) {
        fprintf(stderr,"[ERR]  Could not allocate RAM for xmlinfo\n");
        return -1;
    }

    p = ipod->xmlinfo;
    
    for (i=0; i < npages; i++) {
        if (ipod_scsi_inquiry(ipod, hdr[i+4], buf, sizeof(buf)) < 0) {
            fprintf(stderr,"[ERR]  Sending SCSI Command failed.\n");
            return -1;
        }

        if ((buf[3] + ipod->xmlinfo_len) > psize) {
            fprintf(stderr,"[ERR]  Ran out of memory reading xmlinfo\n");
            free(ipod->xmlinfo);
            ipod->xmlinfo = NULL;
            ipod->xmlinfo_len = 0;
            return -1;
        }

        memcpy(p, buf + 4, buf[3]);
        p += buf[3];
        ipod->xmlinfo_len += buf[3];        
    }

    /* NULL-terminate the XML info */
    *p = 0;

    fprintf(stderr,"[INFO] Read XML info (%d bytes)\n",ipod->xmlinfo_len);

    return 0;
}

void ipod_get_ramsize(struct ipod_t* ipod)
{
    const char needle[] = "<key>RAM</key>\n<integer>";
    char* p;

    if (ipod->xmlinfo == NULL)
        return;

    p = strstr(ipod->xmlinfo, needle);

    if (p) {
        ipod->ramsize = atoi(p + sizeof(needle) - 1);
    }
}

#ifndef RBUTIL

static inline uint32_t getuint32le(unsigned char* buf)
{
  int32_t res = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];

  return res;
}

/* testMarker and GetSecurityBlockKey based on code from BadBlocks and
   Kingstone, posted at http://ipodlinux.org/Flash_Decryption

*/

static bool testMarker(int marker)
{
    int mask, decrypt, temp1, temp2;

    mask = (marker&0xff)|((marker&0xff)<<8)|((marker&0xff)<<16)|((marker&0xff)<<24);
    decrypt = marker ^ mask;
    temp1=(int)((unsigned int)decrypt>>24);
    temp2=decrypt<<8;

    if (temp1==0)
        return false;

    temp2=(int)((unsigned int)temp2>>24);
    decrypt=decrypt<<16;
    decrypt=(int)((unsigned int)decrypt>>24);

    if ((temp1 < temp2) && (temp2 < decrypt))
    {
        temp1 = temp1 & 0xf;
        temp2 = temp2 & 0xf;
        decrypt = decrypt & 0xf;

        if ((temp1 > temp2) && (temp2 > decrypt) && (decrypt != 0))
        {
            return true;
        }
    }
    return false;
}

static int GetSecurityBlockKey(unsigned char *data, unsigned char* this_key)
{
    int constant = 0x54c3a298;
    int key=0;
    int nkeys = 0;
    int aMarker=0;
    int pos=0;
    int c, count;
    int temp1;
    static const int offset[8]={0x5,0x25,0x6f,0x69,0x15,0x4d,0x40,0x34};

    for (c = 0; c < 8; c++)
    {
        pos = offset[c]*4;
        aMarker = getuint32le(data + pos);

        if (testMarker(aMarker))
        {
            if (c<7)
                pos =(offset[c+1]*4)+4;
            else
                pos =(offset[0]*4)+4;

            key=0;

            temp1=aMarker;

            for (count=0;count<2;count++){
                int word = getuint32le(data + pos);
                temp1 = aMarker;
                temp1 = temp1^word;
                temp1 = temp1^constant;
                key = temp1;
                pos = pos+4;
            }
            int r1=0x6f;
            int r2=0;
            int r12;
            int r14;
            unsigned int r_tmp;

            for (count=2;count<128;count=count+2){
                r2=getuint32le(data+count*4);
                r12=getuint32le(data+(count*4)+4);
                r_tmp=(unsigned int)r12>>16;
                r14=r2 | ((int)r_tmp);
                r2=r2&0xffff;
                r2=r2 | r12;
                r1=r1^r14;
                r1=r1+r2;
            }
            key=key^r1;

            // Invert key, little endian
            this_key[0] = key & 0xff;
            this_key[1] = (key >> 8) & 0xff;
            this_key[2] = (key >> 16) & 0xff;
            this_key[3] = (key >> 24) & 0xff;
            nkeys++;
        }
    }
    return nkeys;
}

static int find_key(struct ipod_t* ipod, int aupd, unsigned char* key)
{
    int n;

    /* Firstly read the security block and find the RC4 key.  This is
       in the sector preceeding the AUPD image. */

    fprintf(stderr, "[INFO] Reading security block at offset 0x%08x\n",ipod->ipod_directory[aupd].devOffset-ipod->sector_size);
    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[aupd].devOffset-ipod->sector_size) < 0) {
        return -1;
    }

    if ((n = ipod_read(ipod, ipod_sectorbuf, 512)) < 0) {
        return -1;
    }

    n = GetSecurityBlockKey(ipod_sectorbuf, key);

    if (n != 1)
    {
        fprintf(stderr, "[ERR]  %d keys found in security block, can not continue\n",n);
        return -1;
    }

    return 0;
}

int read_aupd(struct ipod_t* ipod, char* filename)
{
    int length;
    int i;
    int outfile;
    int n;
    int aupd;
    struct rc4_key_t rc4;
    unsigned char key[4];
    unsigned long chksum=0;

    aupd = 0;
    while ((aupd < ipod->nimages) && (ipod->ipod_directory[aupd].ftype != FTYPE_AUPD))
    {
        aupd++;
    }

    if (aupd == ipod->nimages)
    {
        fprintf(stderr,"[ERR]  No AUPD image in firmware partition.\n");
        return -1;
    }

    length = ipod->ipod_directory[aupd].len;

    fprintf(stderr,"[INFO] Reading firmware (%d bytes)\n",length);

    if (find_key(ipod, aupd, key) < 0)
    {
       return -1;
    }

    fprintf(stderr, "[INFO] Decrypting AUPD image with key %02x%02x%02x%02x\n",key[0],key[1],key[2],key[3]);

    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[aupd].devOffset) < 0) {
        return -1;
    }

    i = (length+ipod->sector_size-1) & ~(ipod->sector_size-1);

    if ((n = ipod_read(ipod,ipod_sectorbuf,i)) < 0) {
        return -1;
    }

    if (n < i) {
        fprintf(stderr,"[ERR]  Short read - requested %d bytes, received %d\n",
                i,n);
        return -1;
    }

    /* Perform the decryption - this is standard (A)RC4 */
    matrixArc4Init(&rc4, key, 4);
    matrixArc4(&rc4, ipod_sectorbuf, ipod_sectorbuf, length);

    chksum = 0;
    for (i = 0; i < (int)length; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         chksum += ipod_sectorbuf[i];
    }

    if (chksum != ipod->ipod_directory[aupd].chksum)
    {
        fprintf(stderr,"[ERR]  Decryption failed - checksum error\n");
        return -1;
    }
    fprintf(stderr,"[INFO] Decrypted OK (checksum matches header)\n");

    outfile = open(filename,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666);
    if (outfile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open file %s\n",filename);
        return -1;
    }

    n = write(outfile,ipod_sectorbuf,length);
    if (n != length) {
        fprintf(stderr,"[ERR]  Write error - %d\n",n);
    }
    close(outfile);

    return 0;
}

int write_aupd(struct ipod_t* ipod, char* filename)
{
    unsigned int length;
    int i;
    int x;
    int n;
    int infile;
    int newsize;
    int aupd;
    unsigned long chksum=0;
    struct rc4_key_t rc4;
    unsigned char key[4];

    /* First check that the input file is the correct type for this ipod. */
    infile=open(filename,O_RDONLY);
    if (infile < 0) {
        fprintf(stderr,"[ERR]  Couldn't open input file %s\n",filename);
        return -1;
    }
    
    length = filesize(infile);
    newsize=(length+ipod->sector_size-1)&~(ipod->sector_size-1);

    fprintf(stderr,"[INFO] Padding input file from 0x%08x to 0x%08x bytes\n",
            length,newsize);

    if (newsize > BUFFER_SIZE) {
        fprintf(stderr,"[ERR]  Input file too big for buffer\n");
        if (infile >= 0) close(infile);
        return -1;
    }

    /* Find aupd image number */
    aupd = 0;
    while ((aupd < ipod->nimages) && (ipod->ipod_directory[aupd].ftype != FTYPE_AUPD))
    {
        aupd++;
    }

    if (aupd == ipod->nimages)
    {
        fprintf(stderr,"[ERR]  No AUPD image in firmware partition.\n");
        return -1;
    }

    if (length != ipod->ipod_directory[aupd].len)
    {
        fprintf(stderr,"[ERR]  AUPD image (%d bytes) differs in size to %s (%d bytes).\n",
                       ipod->ipod_directory[aupd].len, filename, length);
        return -1;
    }

    if (find_key(ipod, aupd, key) < 0)
    {
       return -1;
    }

    fprintf(stderr, "[INFO] Encrypting AUPD image with key %02x%02x%02x%02x\n",key[0],key[1],key[2],key[3]);

    /* We now know we have enough space, so write it. */

    fprintf(stderr,"[INFO] Reading input file...\n");
    n = read(infile,ipod_sectorbuf,length);
    if (n < 0) {
        fprintf(stderr,"[ERR]  Couldn't read input file\n");
        close(infile);
        return -1;
    }
    close(infile);

    /* Pad the data with zeros */
    memset(ipod_sectorbuf+length,0,newsize-length);

    /* Calculate the new checksum (before we encrypt) */
    chksum = 0;
    for (i = 0; i < (int)length; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         chksum += ipod_sectorbuf[i];
    }

    /* Perform the encryption - this is standard (A)RC4 */
    matrixArc4Init(&rc4, key, 4);
    matrixArc4(&rc4, ipod_sectorbuf, ipod_sectorbuf, length);

    if (ipod_seek(ipod, ipod->fwoffset+ipod->ipod_directory[aupd].devOffset) < 0) {
        fprintf(stderr,"[ERR]  Seek failed\n");
        return -1;
    }

    if ((n = ipod_write(ipod,ipod_sectorbuf,newsize)) < 0) {
        perror("[ERR]  Write failed\n");
        return -1;
    }

    if (n < newsize) {
        fprintf(stderr,"[ERR]  Short write - requested %d bytes, received %d\n"
                      ,newsize,n);
        return -1;
    }
    fprintf(stderr,"[INFO] Wrote %d bytes to firmware partition\n",n);

    x = ipod->diroffset % ipod->sector_size;

    /* Read directory */
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }

    n=ipod_read(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    /* Update checksum */
    fprintf(stderr,"[INFO] Updating checksum to 0x%08x (was 0x%08x)\n",(unsigned int)chksum,le2int(ipod_sectorbuf + x + aupd*40 + 28));
    int2le(chksum,ipod_sectorbuf+x+aupd*40+28);

    /* Write directory */    
    if (ipod_seek(ipod, ipod->start + ipod->diroffset - x) < 0) { return -1; }
    n=ipod_write(ipod, ipod_sectorbuf, ipod->sector_size);
    if (n < 0) { return -1; }

    return 0;
}

#endif
