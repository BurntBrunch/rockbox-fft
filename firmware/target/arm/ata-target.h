/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Barry Wardell
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

#ifdef CPU_PP

/* primary channel */
#define ATA_DATA        (*((volatile unsigned short*)(IDE_BASE + 0x1e0)))
#define ATA_ERROR       (*((volatile unsigned char*)(IDE_BASE + 0x1e4)))
#define ATA_NSECTOR     (*((volatile unsigned char*)(IDE_BASE + 0x1e8)))
#define ATA_SECTOR      (*((volatile unsigned char*)(IDE_BASE + 0x1ec)))
#define ATA_LCYL        (*((volatile unsigned char*)(IDE_BASE + 0x1f0)))
#define ATA_HCYL        (*((volatile unsigned char*)(IDE_BASE + 0x1f4)))
#define ATA_SELECT      (*((volatile unsigned char*)(IDE_BASE + 0x1f8)))
#define ATA_COMMAND     (*((volatile unsigned char*)(IDE_BASE + 0x1fc)))
#define ATA_CONTROL     (*((volatile unsigned char*)(IDE_BASE + 0x3f8)))

#define STATUS_BSY      0x80
#define STATUS_RDY      0x40
#define STATUS_DF       0x20
#define STATUS_DRQ      0x08
#define STATUS_ERR      0x01
#define ERROR_ABRT      0x04

#define WRITE_PATTERN1  0xa5
#define WRITE_PATTERN2  0x5a
#define WRITE_PATTERN3  0xaa
#define WRITE_PATTERN4  0x55

#define READ_PATTERN1   0xa5
#define READ_PATTERN2   0x5a
#define READ_PATTERN3   0xaa
#define READ_PATTERN4   0x55

#define READ_PATTERN1_MASK 0xff
#define READ_PATTERN2_MASK 0xff
#define READ_PATTERN3_MASK 0xff
#define READ_PATTERN4_MASK 0xff

#if (CONFIG_CPU == PP5002)

#define SET_REG(reg,val) do { reg = (val); \
                              while (!(IDE_CFG_STATUS & 0x40)); \
                         } while (0)
#define SET_16BITREG(reg,val) reg = (val)

/* Plain C reading and writing. See comment in ata-as-arm.S */

#elif defined CPU_PP502x

#define SET_REG(reg,val) reg = (val)
#define SET_16BITREG(reg,val) reg = (val)

/* asm optimized reading and writing */
#define ATA_OPTIMIZED_READING
#define ATA_OPTIMIZED_WRITING
void copy_read_sectors(unsigned char* buf, int wordcount);
void copy_write_sectors(const unsigned char* buf, int wordcount);

#endif /* CONFIG_CPU */

#endif

void ata_reset(void);
bool ata_is_coldstart(void);
void ata_device_init(void);

#ifdef HAVE_ATA_DMA

/* IDE DMA controller registers */
#define IDE_DMA_CONTROL (*(volatile unsigned long *)(0xc3000400))
#define IDE_DMA_LENGTH  (*(volatile unsigned long *)(0xc3000408))
#define IDE_DMA_ADDR    (*(volatile unsigned long *)(0xc300040C))

/* Maximum multi-word DMA mode supported by the controller */
#define ATA_MAX_MWDMA 2

#ifndef BOOTLOADER    
/* The PP5020 supports UDMA 4, but it needs cpu boosting and only
 * improves performance by ~10% with a stock disk.
 * UDMA 2 is stable at 30 Mhz.
 * UDMA 1 is stable at 24 Mhz.
 */
#if CPUFREQ_NORMAL >= 30000000
#define ATA_MAX_UDMA 2
#elif CPUFREQ_NORMAL >= 24000000
#define ATA_MAX_UDMA 1
#else
#error "CPU speeds under 24Mhz have not been tested with DMA"
#endif
#else
/* The bootloader runs at 24 Mhz and needs a slower mode */
#define ATA_MAX_UDMA 1
#endif

void ata_dma_set_mode(unsigned char mode);
bool ata_dma_setup(void *addr, unsigned long bytes, bool write);
bool ata_dma_finish(void);

#endif /* HAVE_ATA_DMA */
