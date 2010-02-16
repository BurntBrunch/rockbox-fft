/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

#include "system.h"
#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"

#   define AV_WL32(p, d) do {                   \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
    } while(0)
#   define AV_WL16(p, d) do {                   \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
    } while(0)

bool get_wave_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    unsigned long totalsamples = 0;
    unsigned long channels = 0;
    unsigned long bitspersample = 0;
    unsigned long numbytes = 0;
    unsigned long offset = 0;
    int read_bytes;
    int i;

    /* get RIFF chunk header */
    if ((lseek(fd, 0, SEEK_SET) < 0)
        || ((read_bytes = read(fd, buf, 12)) < 12))
    {
        return false;
    }
    offset += 12;

    if ((memcmp(buf, "RIFF",4) != 0)
        || (memcmp(&buf[8], "WAVE", 4) !=0 ))
    {
        return false;
    }

    /* iterate over WAVE chunks until 'data' chunk */
    while (true)
    {
        /* get chunk header */
        if ((read_bytes = read(fd, buf, 8)) < 8)
            return false;
        offset += 8;

        /* chunkSize */
        i = get_long_le(&buf[4]);

        if (memcmp(buf, "fmt ", 4) == 0)
        {
            /* get rest of chunk */
            if ((read_bytes = read(fd, buf, 16)) < 16)
                return false;
            offset += 16;
            i -= 16;


            /* skipping wFormatTag */
            /* wChannels */
            channels = buf[2] | (buf[3] << 8);
            /* dwSamplesPerSec */
            id3->frequency = get_long_le(&buf[4]);
            /* dwAvgBytesPerSec */
            id3->bitrate = (get_long_le(&buf[8]) * 8) / 1000;
            /* wBlockAlign */
            id3->bytesperframe = buf[12] | (buf[13] << 8);
            /* wBitsPerSample */
            bitspersample = buf[14] | (buf[15] << 8);
            /* Check for ATRAC3 stream */
            if((buf[0] | (buf[1] << 8)) == 0x0270) 
            {   
                int jsflag = 0;
                if(id3->bitrate == 66 || id3->bitrate == 94)
                    jsflag = 1;

                id3->extradata_size = 14;
                id3->channels = 2;
                id3->codectype = AFMT_OMA_ATRAC3;
               /* Store the extradata for the codec */
                AV_WL16(&id3->id3v2buf[0],  1);             // always 1
                AV_WL32(&id3->id3v2buf[2],  id3->frequency);    // samples rate
                AV_WL16(&id3->id3v2buf[6],  jsflag);        // coding mode
                AV_WL16(&id3->id3v2buf[8],  jsflag);        // coding mode
                AV_WL16(&id3->id3v2buf[10], 1);             // always 1
                AV_WL16(&id3->id3v2buf[12], 0);             // always 0
            }
        }
        else if (memcmp(buf, "data", 4) == 0)
        {
            numbytes = i;
            id3->first_frame_offset = offset;
            break;
        }
        else if (memcmp(buf, "fact", 4) == 0)
        {
            /* dwSampleLength */
            if (i >= 4)
            {
                /* get rest of chunk */
                if ((read_bytes = read(fd, buf, 4)) < 4)
                    return false;
                offset += 4;
                i -= 4;
                totalsamples = get_long_le(buf);
            }
        }

        /* seek to next chunk (even chunk sizes must be padded) */
        if (i & 0x01)
            i++;

        if(lseek(fd, i, SEEK_CUR) < 0)
            return false;
        offset += i;
    }

    if ((numbytes == 0) || (channels == 0))
    {
        return false;
    }

    if (totalsamples == 0)
    {
        /* for PCM only */
        totalsamples = numbytes
            / ((((bitspersample - 1) / 8) + 1) * channels);
    }

    id3->vbr = false;   /* All WAV files are CBR */
    id3->filesize = filesize(fd);

    /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
    if(id3->codectype != AFMT_OMA_ATRAC3)
        id3->length = ((int64_t) totalsamples * 1000) / id3->frequency;
    else
        id3->length   = ((id3->filesize - id3->first_frame_offset) * 8) / id3->bitrate;

    return true;
}
