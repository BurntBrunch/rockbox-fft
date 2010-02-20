/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
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

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "buffering.h"

#include "storage.h"
#include "system.h"
#include "thread.h"
#include "file.h"
#include "panic.h"
#include "memory.h"
#include "lcd.h"
#include "font.h"
#include "button.h"
#include "kernel.h"
#include "tree.h"
#include "debug.h"
#include "sprintf.h"
#include "settings.h"
#include "codecs.h"
#include "audio.h"
#include "mp3_playback.h"
#include "usb.h"
#include "screens.h"
#include "playlist.h"
#include "pcmbuf.h"
#include "bmp.h"
#include "appevents.h"
#include "metadata.h"
#ifdef HAVE_ALBUMART
#include "albumart.h"
#include "jpeg_load.h"
#include "bmp.h"
#endif

#define GUARD_BUFSIZE   (32*1024)

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

/* macros to enable logf for queues
   logging on SYS_TIMEOUT can be disabled */
#ifdef SIMULATOR
/* Define this for logf output of all queuing except SYS_TIMEOUT */
#define BUFFERING_LOGQUEUES
/* Define this to logf SYS_TIMEOUT messages */
/* #define BUFFERING_LOGQUEUES_SYS_TIMEOUT */
#endif

#ifdef BUFFERING_LOGQUEUES
#define LOGFQUEUE logf
#else
#define LOGFQUEUE(...)
#endif

#ifdef BUFFERING_LOGQUEUES_SYS_TIMEOUT
#define LOGFQUEUE_SYS_TIMEOUT logf
#else
#define LOGFQUEUE_SYS_TIMEOUT(...)
#endif

/* default point to start buffer refill */
#define BUFFERING_DEFAULT_WATERMARK      (1024*128)
/* amount of data to read in one read() call */
#define BUFFERING_DEFAULT_FILECHUNK      (1024*32)

#define BUF_HANDLE_MASK                  0x7FFFFFFF


/* assert(sizeof(struct memory_handle)%4==0) */
struct memory_handle {
    int id;                    /* A unique ID for the handle */
    enum data_type type;       /* Type of data buffered with this handle */
    char path[MAX_PATH];       /* Path if data originated in a file */
    int fd;                    /* File descriptor to path (-1 if closed) */
    size_t start;              /* Start index of the handle's data buffer,
                                  for use by reset_handle. */
    size_t data;               /* Start index of the handle's data */
    volatile size_t ridx;      /* Read pointer, relative to the main buffer */
    size_t widx;               /* Write pointer */
    size_t filesize;           /* File total length */
    size_t filerem;            /* Remaining bytes of file NOT in buffer */
    volatile size_t available; /* Available bytes to read from buffer */
    size_t offset;             /* Offset at which we started reading the file */
    struct memory_handle *next;
};
/* invariant: filesize == offset + available + filerem */

static char *buffer;
static char *guard_buffer;

static size_t buffer_len;

static volatile size_t buf_widx;  /* current writing position */
static volatile size_t buf_ridx;  /* current reading position */
/* buf_*idx are values relative to the buffer, not real pointers. */

/* Configuration */
static size_t conf_watermark = 0; /* Level to trigger filebuf fill */
#if MEM > 8
static size_t high_watermark = 0; /* High watermark for rebuffer */
#endif

/* current memory handle in the linked list. NULL when the list is empty. */
static struct memory_handle *cur_handle;
/* first memory handle in the linked list. NULL when the list is empty. */
static struct memory_handle *first_handle;

static int num_handles;  /* number of handles in the list */

static int base_handle_id;

static struct mutex llist_mutex;
static struct mutex llist_mod_mutex;

/* Handle cache (makes find_handle faster).
   This is global so that move_handle and rm_handle can invalidate it. */
static struct memory_handle *cached_handle = NULL;

static struct {
    size_t remaining;   /* Amount of data needing to be buffered */
    size_t wasted;      /* Amount of space available for freeing */
    size_t buffered;    /* Amount of data currently in the buffer */
    size_t useful;      /* Amount of data still useful to the user */
} data_counters;


/* Messages available to communicate with the buffering thread */
enum {
    Q_BUFFER_HANDLE = 1, /* Request buffering of a handle, this should not be
                            used in a low buffer situation. */
    Q_RESET_HANDLE,      /* (internal) Request resetting of a handle to its
                            offset (the offset has to be set beforehand) */
    Q_CLOSE_HANDLE,      /* Request closing a handle */
    Q_BASE_HANDLE,       /* Set the reference handle for buf_useful_data */

    /* Configuration: */
    Q_START_FILL,        /* Request that the buffering thread initiate a buffer
                            fill at its earliest convenience */
    Q_HANDLE_ADDED,      /* Inform the buffering thread that a handle was added,
                            (which means the disk is spinning) */
};

/* Buffering thread */
static void buffering_thread(void);
static long buffering_stack[(DEFAULT_STACK_SIZE + 0x2000)/sizeof(long)];
static const char buffering_thread_name[] = "buffering";
static unsigned int buffering_thread_id = 0;
static struct event_queue buffering_queue;
static struct queue_sender_list buffering_queue_sender_list;



/* Ring buffer helper functions */

static inline uintptr_t ringbuf_offset(const void *ptr)
{
    return (uintptr_t)(ptr - (void*)buffer);
}

/* Buffer pointer (p) plus value (v), wrapped if necessary */
static inline uintptr_t ringbuf_add(uintptr_t p, size_t v)
{
    uintptr_t res = p + v;
    if (res >= buffer_len)
        res -= buffer_len; /* wrap if necssary */
    return res;
}


/* Buffer pointer (p) minus value (v), wrapped if necessary */
static inline uintptr_t ringbuf_sub(uintptr_t p, size_t v)
{
    uintptr_t res = p;
    if (p < v)
        res += buffer_len; /* wrap */
        
    return res - v;
}


/* How far value (v) plus buffer pointer (p1) will cross buffer pointer (p2) */
static inline ssize_t ringbuf_add_cross(uintptr_t p1, size_t v, uintptr_t p2)
{
    ssize_t res = p1 + v - p2;
    if (p1 >= p2) /* wrap if necessary */
        res -= buffer_len;

    return res;
}

/* Bytes available in the buffer */
#define BUF_USED ringbuf_sub(buf_widx, buf_ridx)

/*
LINKED LIST MANAGEMENT
======================

add_handle  : Add a handle to the list
rm_handle   : Remove a handle from the list
find_handle : Get a handle pointer from an ID
move_handle : Move a handle in the buffer (with or without its data)

These functions only handle the linked list structure. They don't touch the
contents of the struct memory_handle headers. They also change the buf_*idx
pointers when necessary and manage the handle IDs.

The first and current (== last) handle are kept track of.
A new handle is added at buf_widx and becomes the current one.
buf_widx always points to the current writing position for the current handle
buf_ridx always points to the location of the first handle.
buf_ridx == buf_widx means the buffer is empty.
*/


/* Add a new handle to the linked list and return it. It will have become the
   new current handle.
   data_size must contain the size of what will be in the handle.
   can_wrap tells us whether this type of data may wrap on buffer
   alloc_all tells us if we must immediately be able to allocate data_size
   returns a valid memory handle if all conditions for allocation are met.
           NULL if there memory_handle itself cannot be allocated or if the
           data_size cannot be allocated and alloc_all is set.  This function's
           only potential side effect is to allocate space for the cur_handle
           if it returns NULL.
   */
static struct memory_handle *add_handle(size_t data_size, bool can_wrap,
                                        bool alloc_all)
{
    /* gives each handle a unique id */
    static int cur_handle_id = 0;
    size_t shift;
    size_t new_widx;
    size_t len;
    int overlap;

    if (num_handles >= BUF_MAX_HANDLES)
        return NULL;

    mutex_lock(&llist_mutex);
    mutex_lock(&llist_mod_mutex);

    if (cur_handle && cur_handle->filerem > 0) {
        /* the current handle hasn't finished buffering. We can only add
           a new one if there is already enough free space to finish
           the buffering. */
        size_t req = cur_handle->filerem + sizeof(struct memory_handle);
        if (ringbuf_add_cross(cur_handle->widx, req, buf_ridx) >= 0) {
            /* Not enough space */
            mutex_unlock(&llist_mod_mutex);
            mutex_unlock(&llist_mutex);
            return NULL;
        } else {
            /* Allocate the remainder of the space for the current handle */
            buf_widx = ringbuf_add(cur_handle->widx, cur_handle->filerem);
        }
    }

    /* align to 4 bytes up */
    new_widx = ringbuf_add(buf_widx, 3) & ~3;

    len = data_size + sizeof(struct memory_handle);

    /* First, will the handle wrap? */
    /* If the handle would wrap, move to the beginning of the buffer,
     * or if the data must not but would wrap, move it to the beginning */
    if( (new_widx + sizeof(struct memory_handle) > buffer_len) ||
                   (!can_wrap && (new_widx + len > buffer_len)) ) {
        new_widx = 0;
    }

    /* How far we shifted buf_widx to align things, must be < buffer_len */
    shift = ringbuf_sub(new_widx, buf_widx);

    /* How much space are we short in the actual ring buffer? */
    overlap = ringbuf_add_cross(buf_widx, shift + len, buf_ridx);
    if (overlap >= 0 && (alloc_all || (unsigned)overlap > data_size)) {
        /* Not enough space for required allocations */
        mutex_unlock(&llist_mod_mutex);
        mutex_unlock(&llist_mutex);
        return NULL;
    }

    /* There is enough space for the required data, advance the buf_widx and
     * initialize the struct */
    buf_widx = new_widx;

    struct memory_handle *new_handle =
        (struct memory_handle *)(&buffer[buf_widx]);

    /* only advance the buffer write index of the size of the struct */
    buf_widx = ringbuf_add(buf_widx, sizeof(struct memory_handle));

    new_handle->id = cur_handle_id;
    /* Wrap signed int is safe and 0 doesn't happen */
    cur_handle_id = (cur_handle_id + 1) & BUF_HANDLE_MASK;
    new_handle->next = NULL;
    num_handles++;

    if (!first_handle)
        /* the new handle is the first one */
        first_handle = new_handle;

    if (cur_handle)
        cur_handle->next = new_handle;

    cur_handle = new_handle;

    mutex_unlock(&llist_mod_mutex);
    mutex_unlock(&llist_mutex);
    return new_handle;
}

/* Delete a given memory handle from the linked list
   and return true for success. Nothing is actually erased from memory. */
static bool rm_handle(const struct memory_handle *h)
{
    if (h == NULL)
        return true;

    mutex_lock(&llist_mutex);
    mutex_lock(&llist_mod_mutex);

    if (h == first_handle) {
        first_handle = h->next;
        if (h == cur_handle) {
            /* h was the first and last handle: the buffer is now empty */
            cur_handle = NULL;
            buf_ridx = buf_widx = 0;
        } else {
            /* update buf_ridx to point to the new first handle */
            buf_ridx = (size_t)ringbuf_offset(first_handle);
        }
    } else {
        struct memory_handle *m = first_handle;
        /* Find the previous handle */
        while (m && m->next != h) {
            m = m->next;
        }
        if (m && m->next == h) {
            m->next = h->next;
            if (h == cur_handle) {
                cur_handle = m;
                buf_widx = cur_handle->widx;
            }
        } else {
            mutex_unlock(&llist_mod_mutex);
            mutex_unlock(&llist_mutex);
            return false;
        }
    }

    /* Invalidate the cache to prevent it from keeping the old location of h */
    if (h == cached_handle)
        cached_handle = NULL;

    num_handles--;

    mutex_unlock(&llist_mod_mutex);
    mutex_unlock(&llist_mutex);
    return true;
}

/* Return a pointer to the memory handle of given ID.
   NULL if the handle wasn't found */
static struct memory_handle *find_handle(int handle_id)
{
    if (handle_id < 0)
        return NULL;

    mutex_lock(&llist_mutex);

    /* simple caching because most of the time the requested handle
    will either be the same as the last, or the one after the last */
    if (cached_handle)
    {
        if (cached_handle->id == handle_id) {
            mutex_unlock(&llist_mutex);
            return cached_handle;
        } else if (cached_handle->next &&
                   (cached_handle->next->id == handle_id)) {
            cached_handle = cached_handle->next;
            mutex_unlock(&llist_mutex);
            return cached_handle;
        }
    }

    struct memory_handle *m = first_handle;
    while (m && m->id != handle_id) {
        m = m->next;
    }
    /* This condition can only be reached with !m or m->id == handle_id */
    if (m)
        cached_handle = m;

    mutex_unlock(&llist_mutex);
    return m;
}

/* Move a memory handle and data_size of its data delta bytes along the buffer.
   delta maximum bytes available to move the handle.  If the move is performed
         it is set to the actual distance moved.
   data_size is the amount of data to move along with the struct.
   returns a valid memory_handle if the move is successful
           NULL if the handle is NULL, the  move would be less than the size of
           a memory_handle after correcting for wraps or if the handle is not
           found in the linked list for adjustment.  This function has no side
           effects if NULL is returned. */
static bool move_handle(struct memory_handle **h, size_t *delta,
                        size_t data_size, bool can_wrap)
{
    struct memory_handle *dest;
    const struct memory_handle *src;
    int32_t *here;
    int32_t *there;
    int32_t *end;
    int32_t *begin;
    size_t final_delta = *delta, size_to_move, n;
    uintptr_t oldpos, newpos;
    intptr_t overlap, overlap_old;

    if (h == NULL || (src = *h) == NULL)
        return false;

    size_to_move = sizeof(struct memory_handle) + data_size;

    /* Align to four bytes, down */
    final_delta &= ~3;
    if (final_delta < sizeof(struct memory_handle)) {
        /* It's not legal to move less than the size of the struct */
        return false;
    }

    mutex_lock(&llist_mutex);
    mutex_lock(&llist_mod_mutex);

    oldpos = ringbuf_offset(src);
    newpos = ringbuf_add(oldpos, final_delta);
    overlap = ringbuf_add_cross(newpos, size_to_move, buffer_len - 1);
    overlap_old = ringbuf_add_cross(oldpos, size_to_move, buffer_len -1);

    if (overlap > 0) {
        /* Some part of the struct + data would wrap, maybe ok */
        size_t correction = 0;
        /* If the overlap lands inside the memory_handle */
        if (!can_wrap) {
            /* Otherwise the overlap falls in the data area and must all be
             * backed out.  This may become conditional if ever we move
             * data that is allowed to wrap (ie audio) */
            correction = overlap;
        } else if ((uintptr_t)overlap > data_size) {
            /* Correct the position and real delta to prevent the struct from
             * wrapping, this guarantees an aligned delta, I think */
            correction = overlap - data_size;
        }
        if (correction) {
            /* Align correction to four bytes up */
            correction = (correction + 3) & ~3;
            if (final_delta < correction + sizeof(struct memory_handle)) {
                /* Delta cannot end up less than the size of the struct */
                mutex_unlock(&llist_mod_mutex);
                mutex_unlock(&llist_mutex);
                return false;
            }
            newpos -= correction;
            overlap -= correction;/* Used below to know how to split the data */
            final_delta -= correction;
        }
    }

    dest = (struct memory_handle *)(&buffer[newpos]);

    if (src == first_handle) {
        first_handle = dest;
        buf_ridx = newpos;
    } else {
        struct memory_handle *m = first_handle;
        while (m && m->next != src) {
            m = m->next;
        }
        if (m && m->next == src) {
            m->next = dest;
        } else {
            mutex_unlock(&llist_mod_mutex);
            mutex_unlock(&llist_mutex);
            return false;
        }
    }


    /* Update the cache to prevent it from keeping the old location of h */
    if (src == cached_handle)
        cached_handle = dest;

    /* the cur_handle pointer might need updating */
    if (src == cur_handle)
        cur_handle = dest;


    /* Copying routine takes into account that the handles have a
     * distance between each other which is a multiple of four.  Faster 2 word
     * copy may be ok but do this for safety and because wrapped copies should
     * be fairly uncommon */

    here = (int32_t *)((ringbuf_add(oldpos, size_to_move - 1) & ~3)+ (intptr_t)buffer);
    there =(int32_t *)((ringbuf_add(newpos, size_to_move - 1) & ~3)+ (intptr_t)buffer);
    end = (int32_t *)(( intptr_t)buffer + buffer_len - 4);
    begin =(int32_t *)buffer;

    n = (size_to_move & ~3)/4;

    if ( overlap_old > 0 || overlap > 0 ) {
    /* Old or moved handle wraps */
        while (n--) {
            if (here < begin)
                here =  end;
            if (there < begin)
                there = end;
           *there-- = *here--;
        }
    } else {
    /* both handles do not wrap */
        memmove(dest,src,size_to_move);
    }


    /* Update the caller with the new location of h and the distance moved */
    *h = dest;
    *delta = final_delta;
    mutex_unlock(&llist_mod_mutex);
    mutex_unlock(&llist_mutex);
    return dest;
}


/*
BUFFER SPACE MANAGEMENT
=======================

update_data_counters: Updates the values in data_counters
buffer_is_low   : Returns true if the amount of useful data in the buffer is low
buffer_handle   : Buffer data for a handle
reset_handle    : Reset write position and data buffer of a handle to its offset
rebuffer_handle : Seek to a nonbuffered part of a handle by rebuffering the data
shrink_handle   : Free buffer space by moving a handle
fill_buffer     : Call buffer_handle for all handles that have data to buffer

These functions are used by the buffering thread to manage buffer space.
*/

static void update_data_counters(void)
{
    struct memory_handle *m = find_handle(base_handle_id);
    bool is_useful = m==NULL;

    size_t buffered = 0;
    size_t wasted = 0;
    size_t remaining = 0;
    size_t useful = 0;

    mutex_lock(&llist_mutex);

    m = first_handle;
    while (m) {
        buffered += m->available;
        wasted += ringbuf_sub(m->ridx, m->data);
        remaining += m->filerem;

        if (m->id == base_handle_id)
            is_useful = true;

        if (is_useful)
            useful += ringbuf_sub(m->widx, m->ridx);

        m = m->next;
    }

    mutex_unlock(&llist_mutex);

    data_counters.buffered = buffered;
    data_counters.wasted = wasted;
    data_counters.remaining = remaining;
    data_counters.useful = useful;
}

static inline bool buffer_is_low(void)
{
    update_data_counters();
    return data_counters.useful < (conf_watermark / 2);
}

/* Buffer data for the given handle.
   Return whether or not the buffering should continue explicitly.  */
static bool buffer_handle(int handle_id)
{
    logf("buffer_handle(%d)", handle_id);
    struct memory_handle *h = find_handle(handle_id);
    bool stop = false;

    if (!h)
        return true;

    if (h->filerem == 0) {
        /* nothing left to buffer */
        return true;
    }

    if (h->fd < 0)  /* file closed, reopen */
    {
        if (*h->path)
            h->fd = open(h->path, O_RDONLY);

        if (h->fd < 0)
        {
            /* could not open the file, truncate it where it is */
            h->filesize -= h->filerem;
            h->filerem = 0;
            return true;
        }

        if (h->offset)
            lseek(h->fd, h->offset, SEEK_SET);
    }

    trigger_cpu_boost();

    if (h->type == TYPE_ID3)
    {
        if (!get_metadata((struct mp3entry *)(buffer + h->data), h->fd, h->path))
        {
            /* metadata parsing failed: clear the buffer. */
            memset(buffer + h->data, 0, sizeof(struct mp3entry));
        }
        close(h->fd);
        h->fd = -1;
        h->filerem = 0;
        h->available = sizeof(struct mp3entry);
        h->widx += sizeof(struct mp3entry);
        send_event(BUFFER_EVENT_FINISHED, &h->id);
        return true;
    }

    while (h->filerem > 0 && !stop)
    {
        /* max amount to copy */
        size_t copy_n = MIN( MIN(h->filerem, BUFFERING_DEFAULT_FILECHUNK),
                             buffer_len - h->widx);

        ssize_t overlap;
        uintptr_t next_handle = ringbuf_offset(h->next);

        /* stop copying if it would overwrite the reading position */
        if (ringbuf_add_cross(h->widx, copy_n, buf_ridx) >= 0)
            return false;

        /* FIXME: This would overwrite the next handle
         * If this is true, then there's a handle even though we have still
         * data to buffer. This should NEVER EVER happen! (but it does :( ) */
        if (h->next && (overlap
                = ringbuf_add_cross(h->widx, copy_n, next_handle)) > 0)
        {
            /* stop buffering data for now and post-pone buffering the rest */
            stop = true;
            DEBUGF( "%s(): Preventing handle corruption: h1.id:%d h2.id:%d"
                    " copy_n:%lu overlap:%ld h1.filerem:%lu\n", __func__,
                    h->id, h->next->id, (unsigned long)copy_n, overlap,
                    (unsigned long)h->filerem);
            copy_n -= overlap;
        }

        /* rc is the actual amount read */
        int rc = read(h->fd, &buffer[h->widx], copy_n);

        if (rc < 0)
        {
            /* Some kind of filesystem error, maybe recoverable if not codec */
            if (h->type == TYPE_CODEC) {
                logf("Partial codec");
                break;
            }

            DEBUGF("File ended %ld bytes early\n", (long)h->filerem);
            h->filesize -= h->filerem;
            h->filerem = 0;
            break;
        }

        /* Advance buffer */
        h->widx = ringbuf_add(h->widx, rc);
        if (h == cur_handle)
            buf_widx = h->widx;
        h->available += rc;
        h->filerem -= rc;

        /* If this is a large file, see if we need to break or give the codec
         * more time */
        if (h->type == TYPE_PACKET_AUDIO &&
            pcmbuf_is_lowdata() && !buffer_is_low())
        {
            sleep(1);
        }
        else
        {
            yield();
        }

        if (!queue_empty(&buffering_queue))
            break;
    }

    if (h->filerem == 0) {
        /* finished buffering the file */
        close(h->fd);
        h->fd = -1;
        send_event(BUFFER_EVENT_FINISHED, &h->id);
    }

    return !stop;
}

/* Reset writing position and data buffer of a handle to its current offset.
   Use this after having set the new offset to use. */
static void reset_handle(int handle_id)
{
    size_t alignment_pad;

    logf("reset_handle(%d)", handle_id);

    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return;

    /* Align to desired storage alignment */
    alignment_pad = (h->offset - (size_t)(&buffer[h->start]))
                    & STORAGE_ALIGN_MASK;
    h->ridx = h->widx = h->data = ringbuf_add(h->start, alignment_pad);

    if (h == cur_handle)
        buf_widx = h->widx;
    h->available = 0;
    h->filerem = h->filesize - h->offset;

    if (h->fd >= 0) {
        lseek(h->fd, h->offset, SEEK_SET);
    }
}

/* Seek to a nonbuffered part of a handle by rebuffering the data. */
static void rebuffer_handle(int handle_id, size_t newpos)
{
    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return;

    /* When seeking foward off of the buffer, if it is a short seek don't
       rebuffer the whole track, just read enough to satisfy */
    if (newpos > h->offset && newpos - h->offset < BUFFERING_DEFAULT_FILECHUNK)
    {
        LOGFQUEUE("buffering >| Q_BUFFER_HANDLE %d", handle_id);
        queue_send(&buffering_queue, Q_BUFFER_HANDLE, handle_id);
        h->ridx = h->data + newpos;
        return;
    }

    h->offset = newpos;

    /* Reset the handle to its new offset */
    LOGFQUEUE("buffering >| Q_RESET_HANDLE %d", handle_id);
    queue_send(&buffering_queue, Q_RESET_HANDLE, handle_id);

    uintptr_t next = ringbuf_offset(h->next);
    if (ringbuf_sub(next, h->data) < h->filesize - newpos)
    {
        /* There isn't enough space to rebuffer all of the track from its new
           offset, so we ask the user to free some */
        DEBUGF("%s(): space is needed\n", __func__);
        send_event(BUFFER_EVENT_REBUFFER, &handle_id);
    }

    /* Now we ask for a rebuffer */
    LOGFQUEUE("buffering >| Q_BUFFER_HANDLE %d", handle_id);
    queue_send(&buffering_queue, Q_BUFFER_HANDLE, handle_id);
}

static bool close_handle(int handle_id)
{
    struct memory_handle *h = find_handle(handle_id);

    /* If the handle is not found, it is closed */
    if (!h)
        return true;

    if (h->fd >= 0) {
        close(h->fd);
        h->fd = -1;
    }

    /* rm_handle returns true unless the handle somehow persists after exit */
    return rm_handle(h);
}

/* Free buffer space by moving the handle struct right before the useful
   part of its data buffer or by moving all the data. */
static void shrink_handle(struct memory_handle *h)
{
    size_t delta;

    if (!h)
        return;

    if (h->next && h->filerem == 0 &&
            (h->type == TYPE_ID3 || h->type == TYPE_CUESHEET ||
             h->type == TYPE_BITMAP || h->type == TYPE_CODEC ||
             h->type == TYPE_ATOMIC_AUDIO))
    {
        /* metadata handle: we can move all of it */
        uintptr_t handle_distance =
            ringbuf_sub(ringbuf_offset(h->next), h->data);
        delta = handle_distance - h->available;

        /* The value of delta might change for alignment reasons */
        if (!move_handle(&h, &delta, h->available, h->type==TYPE_CODEC))
            return;

        size_t olddata = h->data;
        h->data = ringbuf_add(h->data, delta);
        h->ridx = ringbuf_add(h->ridx, delta);
        h->widx = ringbuf_add(h->widx, delta);

        if (h->type == TYPE_ID3 && h->filesize == sizeof(struct mp3entry)) {
            /* when moving an mp3entry we need to readjust its pointers. */
            adjust_mp3entry((struct mp3entry *)&buffer[h->data],
                            (void *)&buffer[h->data],
                            (const void *)&buffer[olddata]);
        } else if (h->type == TYPE_BITMAP) {
            /* adjust the bitmap's pointer */
            struct bitmap *bmp = (struct bitmap *)&buffer[h->data];
            bmp->data = &buffer[h->data + sizeof(struct bitmap)];
        }
    }
    else
    {
        /* only move the handle struct */
        delta = ringbuf_sub(h->ridx, h->data);
        if (!move_handle(&h, &delta, 0, true))
            return;

        h->data = ringbuf_add(h->data, delta);
        h->start = ringbuf_add(h->start, delta);
        h->available -= delta;
        h->offset += delta;
    }
}

/* Fill the buffer by buffering as much data as possible for handles that still
   have data left to buffer
   Return whether or not to continue filling after this */
static bool fill_buffer(void)
{
    logf("fill_buffer()");
    struct memory_handle *m;
    shrink_handle(first_handle);
    m = first_handle;
    while (queue_empty(&buffering_queue) && m) {
        if (m->filerem > 0) {
            if (!buffer_handle(m->id)) {
                m = NULL;
                break;
            }
        }
        m = m->next;
    }

    if (m) {
        return true;
    }
    else
    {
        /* only spin the disk down if the filling wasn't interrupted by an
           event arriving in the queue. */
        storage_sleep();
        return false;
    }
}

#ifdef HAVE_ALBUMART
/* Given a file descriptor to a bitmap file, write the bitmap data to the
   buffer, with a struct bitmap and the actual data immediately following.
   Return value is the total size (struct + data). */
static int load_image(int fd, const char *path, struct dim *dim)
{
    int rc;
    struct bitmap *bmp = (struct bitmap *)&buffer[buf_widx];

    /* get the desired image size */
    bmp->width = dim->width, bmp->height = dim->height;
    /* FIXME: alignment may be needed for the data buffer. */
    bmp->data = &buffer[buf_widx + sizeof(struct bitmap)];
#ifndef HAVE_JPEG
    (void) path;
#endif
#if (LCD_DEPTH > 1) || defined(HAVE_REMOTE_LCD) && (LCD_REMOTE_DEPTH > 1)
    bmp->maskdata = NULL;
#endif

    int free = (int)MIN(buffer_len - BUF_USED, buffer_len - buf_widx)
                               - sizeof(struct bitmap);

#ifdef HAVE_JPEG
    int pathlen = strlen(path);
    if (strcmp(path + pathlen - 4, ".bmp"))
        rc = read_jpeg_fd(fd, bmp, free, FORMAT_NATIVE|FORMAT_DITHER|
                         FORMAT_RESIZE|FORMAT_KEEP_ASPECT, NULL);
    else
#endif
        rc = read_bmp_fd(fd, bmp, free, FORMAT_NATIVE|FORMAT_DITHER|
                         FORMAT_RESIZE|FORMAT_KEEP_ASPECT, NULL);
    return rc + (rc > 0 ? sizeof(struct bitmap) : 0);
}
#endif


/*
MAIN BUFFERING API CALLS
========================

bufopen     : Request the opening of a new handle for a file
bufalloc    : Open a new handle for data other than a file.
bufclose    : Close an open handle
bufseek     : Set the read pointer in a handle
bufadvance  : Move the read pointer in a handle
bufread     : Copy data from a handle into a given buffer
bufgetdata  : Give a pointer to the handle's data

These functions are exported, to allow interaction with the buffer.
They take care of the content of the structs, and rely on the linked list
management functions for all the actual handle management work.
*/


/* Reserve space in the buffer for a file.
   filename: name of the file to open
   offset: offset at which to start buffering the file, useful when the first
           (offset-1) bytes of the file aren't needed.
   type: one of the data types supported (audio, image, cuesheet, others
   user_data: user data passed possibly passed in subcalls specific to a
              data_type (only used for image (albumart) buffering so far )
   return value: <0 if the file cannot be opened, or one file already
   queued to be opened, otherwise the handle for the file in the buffer
*/
int bufopen(const char *file, size_t offset, enum data_type type,
            void *user_data)
{
#ifndef HAVE_ALBUMART
    /* currently only used for aa loading */
    (void)user_data;
#endif
    if (type == TYPE_ID3)
    {
        /* ID3 case: allocate space, init the handle and return. */

        struct memory_handle *h = add_handle(sizeof(struct mp3entry), false, true);
        if (!h)
            return ERR_BUFFER_FULL;

        h->fd = -1;
        h->filesize = sizeof(struct mp3entry);
        h->filerem = sizeof(struct mp3entry);
        h->offset = 0;
        h->data = buf_widx;
        h->ridx = buf_widx;
        h->widx = buf_widx;
        h->available = 0;
        h->type = type;
        strlcpy(h->path, file, MAX_PATH);

        buf_widx += sizeof(struct mp3entry);  /* safe because the handle
                                                 can't wrap */

        /* Inform the buffering thread that we added a handle */
        LOGFQUEUE("buffering > Q_HANDLE_ADDED %d", h->id);
        queue_post(&buffering_queue, Q_HANDLE_ADDED, h->id);

        return h->id;
    }

    /* Other cases: there is a little more work. */

    int fd = open(file, O_RDONLY);
    if (fd < 0)
        return ERR_FILE_ERROR;

    size_t size = filesize(fd);
    bool can_wrap = type==TYPE_PACKET_AUDIO || type==TYPE_CODEC;

    size_t adjusted_offset = offset;
    if (adjusted_offset > size)
        adjusted_offset = 0;

    /* Reserve extra space because alignment can move data forward */
    struct memory_handle *h = add_handle(size-adjusted_offset+STORAGE_ALIGN_MASK,
                                         can_wrap, false);
    if (!h)
    {
        DEBUGF("%s(): failed to add handle\n", __func__);
        close(fd);
        return ERR_BUFFER_FULL;
    }

    strlcpy(h->path, file, MAX_PATH);
    h->offset = adjusted_offset;

    /* Don't bother to storage align bitmaps because they are not
     * loaded directly into the buffer.
     */
    if (type != TYPE_BITMAP)
    {
        size_t alignment_pad;
        
        /* Remember where data area starts, for use by reset_handle */
        h->start = buf_widx;

        /* Align to desired storage alignment */
        alignment_pad = (adjusted_offset - (size_t)(&buffer[buf_widx]))
                        & STORAGE_ALIGN_MASK;
        buf_widx = ringbuf_add(buf_widx, alignment_pad);
    }

    h->ridx = buf_widx;
    h->widx = buf_widx;
    h->data = buf_widx;
    h->available = 0;
    h->filerem = 0;
    h->type = type;

#ifdef HAVE_ALBUMART
    if (type == TYPE_BITMAP)
    {
        /* Bitmap file: we load the data instead of the file */
        int rc;
        mutex_lock(&llist_mod_mutex); /* Lock because load_bitmap yields */
        rc = load_image(fd, file, (struct dim*)user_data);
        mutex_unlock(&llist_mod_mutex);
        if (rc <= 0)
        {
            rm_handle(h);
            close(fd);
            return ERR_FILE_ERROR;
        }
        h->filerem = 0;
        h->filesize = rc;
        h->available = rc;
        h->widx = buf_widx + rc; /* safe because the data doesn't wrap */
        buf_widx += rc;  /* safe too */
    }
    else
#endif
    {
        h->filerem = size - adjusted_offset;
        h->filesize = size;
        h->available = 0;
        h->widx = buf_widx;
    }

    if (type == TYPE_CUESHEET) {
        h->fd = fd;
        /* Immediately start buffering those */
        LOGFQUEUE("buffering >| Q_BUFFER_HANDLE %d", h->id);
        queue_send(&buffering_queue, Q_BUFFER_HANDLE, h->id);
    } else {
        /* Other types will get buffered in the course of normal operations */
        h->fd = -1;
        close(fd);

        /* Inform the buffering thread that we added a handle */
        LOGFQUEUE("buffering > Q_HANDLE_ADDED %d", h->id);
        queue_post(&buffering_queue, Q_HANDLE_ADDED, h->id);
    }

    logf("bufopen: new hdl %d", h->id);
    return h->id;
}

/* Open a new handle from data that needs to be copied from memory.
   src is the source buffer from which to copy data. It can be NULL to simply
   reserve buffer space.
   size is the requested size. The call will only be successful if the
   requested amount of data can entirely fit in the buffer without wrapping.
   Return value is the handle id for success or <0 for failure.
*/
int bufalloc(const void *src, size_t size, enum data_type type)
{
    struct memory_handle *h = add_handle(size, false, true);

    if (!h)
        return ERR_BUFFER_FULL;

    if (src) {
        if (type == TYPE_ID3 && size == sizeof(struct mp3entry)) {
            /* specially take care of struct mp3entry */
            copy_mp3entry((struct mp3entry *)&buffer[buf_widx],
                          (const struct mp3entry *)src);
        } else {
            memcpy(&buffer[buf_widx], src, size);
        }
    }

    h->fd = -1;
    *h->path = 0;
    h->filesize = size;
    h->filerem = 0;
    h->offset = 0;
    h->ridx = buf_widx;
    h->widx = buf_widx + size; /* this is safe because the data doesn't wrap */
    h->data = buf_widx;
    h->available = size;
    h->type = type;

    buf_widx += size;  /* safe too */

    logf("bufalloc: new hdl %d", h->id);
    return h->id;
}

/* Close the handle. Return true for success and false for failure */
bool bufclose(int handle_id)
{
    logf("bufclose(%d)", handle_id);

    LOGFQUEUE("buffering >| Q_CLOSE_HANDLE %d", handle_id);
    return queue_send(&buffering_queue, Q_CLOSE_HANDLE, handle_id);
}

/* Set reading index in handle (relatively to the start of the file).
   Access before the available data will trigger a rebuffer.
   Return 0 for success and < 0 for failure:
     -1 if the handle wasn't found
     -2 if the new requested position was beyond the end of the file
*/
int bufseek(int handle_id, size_t newpos)
{
    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (newpos > h->filesize) {
        /* access beyond the end of the file */
        return ERR_INVALID_VALUE;
    }
    else if (newpos < h->offset || h->offset + h->available < newpos) {
        /* access before or after buffered data. A rebuffer is needed. */
        rebuffer_handle(handle_id, newpos);
    }
    else {
        h->ridx = ringbuf_add(h->data, newpos - h->offset);
    }
    return 0;
}

/* Advance the reading index in a handle (relatively to its current position).
   Return 0 for success and < 0 for failure */
int bufadvance(int handle_id, off_t offset)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    size_t newpos = h->offset + ringbuf_sub(h->ridx, h->data) + offset;
    return bufseek(handle_id, newpos);
}

/* Used by bufread and bufgetdata to prepare the buffer and retrieve the
 * actual amount of data available for reading.  This function explicitly
 * does not check the validity of the input handle.  It does do range checks
 * on size and returns a valid (and explicit) amount of data for reading */
static struct memory_handle *prep_bufdata(int handle_id, size_t *size,
                                          bool guardbuf_limit)
{
    struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return NULL;

    size_t avail = ringbuf_sub(h->widx, h->ridx);

    if (avail == 0 && h->filerem == 0)
    {
        /* File is finished reading */
        *size = 0;
        return h;
    }

    if (*size == 0 || *size > avail + h->filerem)
        *size = avail + h->filerem;

    if (guardbuf_limit && h->type == TYPE_PACKET_AUDIO && *size > GUARD_BUFSIZE)
    {
        logf("data request > guardbuf");
        /* If more than the size of the guardbuf is requested and this is a
         * bufgetdata, limit to guard_bufsize over the end of the buffer */
        *size = MIN(*size, buffer_len - h->ridx + GUARD_BUFSIZE);
        /* this ensures *size <= buffer_len - h->ridx + GUARD_BUFSIZE */
    }

    if (h->filerem > 0 && avail < *size)
    {
        /* Data isn't ready. Request buffering */
        buf_request_buffer_handle(handle_id);
        /* Wait for the data to be ready */
        do
        {
            sleep(1);
            /* it is not safe for a non-buffering thread to sleep while
             * holding a handle */
            h = find_handle(handle_id);
            if (!h)
                return NULL;
            avail = ringbuf_sub(h->widx, h->ridx);
        }
        while (h->filerem > 0 && avail < *size);
    }

    *size = MIN(*size,avail);
    return h;
}

/* Copy data from the given handle to the dest buffer.
   Return the number of bytes copied or < 0 for failure (handle not found).
   The caller is blocked until the requested amount of data is available.
*/
ssize_t bufread(int handle_id, size_t size, void *dest)
{
    const struct memory_handle *h;
    size_t adjusted_size = size;

    h = prep_bufdata(handle_id, &adjusted_size, false);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (h->ridx + adjusted_size > buffer_len)
    {
        /* the data wraps around the end of the buffer */
        size_t read = buffer_len - h->ridx;
        memcpy(dest, &buffer[h->ridx], read);
        memcpy(dest+read, buffer, adjusted_size - read);
    }
    else
    {
        memcpy(dest, &buffer[h->ridx], adjusted_size);
    }

    return adjusted_size;
}

/* Update the "data" pointer to make the handle's data available to the caller.
   Return the length of the available linear data or < 0 for failure (handle
   not found).
   The caller is blocked until the requested amount of data is available.
   size is the amount of linear data requested. it can be 0 to get as
   much as possible.
   The guard buffer may be used to provide the requested size. This means it's
   unsafe to request more than the size of the guard buffer.
*/
ssize_t bufgetdata(int handle_id, size_t size, void **data)
{
    const struct memory_handle *h;
    size_t adjusted_size = size;

    h = prep_bufdata(handle_id, &adjusted_size, true);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (h->ridx + adjusted_size > buffer_len)
    {
        /* the data wraps around the end of the buffer :
           use the guard buffer to provide the requested amount of data. */
        size_t copy_n = h->ridx + adjusted_size - buffer_len;
        /* prep_bufdata ensures adjusted_size <= buffer_len - h->ridx + GUARD_BUFSIZE,
           so copy_n <= GUARD_BUFSIZE */
        memcpy(guard_buffer, (const unsigned char *)buffer, copy_n);
    }

    if (data)
        *data = &buffer[h->ridx];

    return adjusted_size;
}

ssize_t bufgettail(int handle_id, size_t size, void **data)
{
    size_t tidx;

    const struct memory_handle *h;

    h = find_handle(handle_id);

    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (h->filerem)
        return ERR_HANDLE_NOT_DONE;

    /* We don't support tail requests of > guardbuf_size, for simplicity */
    if (size > GUARD_BUFSIZE)
        return ERR_INVALID_VALUE;

    tidx = ringbuf_sub(h->widx, size);

    if (tidx + size > buffer_len)
    {
        size_t copy_n = tidx + size - buffer_len;
        memcpy(guard_buffer, (const unsigned char *)buffer, copy_n);
    }

    *data = &buffer[tidx];
    return size;
}

ssize_t bufcuttail(int handle_id, size_t size)
{
    struct memory_handle *h;
    size_t adjusted_size = size;

    h = find_handle(handle_id);

    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    if (h->filerem)
        return ERR_HANDLE_NOT_DONE;

    if (h->available < adjusted_size)
        adjusted_size = h->available;

    h->available -= adjusted_size;
    h->filesize -= adjusted_size;
    h->widx = ringbuf_sub(h->widx, adjusted_size);
    if (h == cur_handle)
        buf_widx = h->widx;

    return adjusted_size;
}


/*
SECONDARY EXPORTED FUNCTIONS
============================

buf_get_offset
buf_handle_offset
buf_request_buffer_handle
buf_set_base_handle
buf_used
register_buffering_callback
unregister_buffering_callback

These functions are exported, to allow interaction with the buffer.
They take care of the content of the structs, and rely on the linked list
management functions for all the actual handle management work.
*/

/* Get a handle offset from a pointer */
ssize_t buf_get_offset(int handle_id, void *ptr)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;

    return (size_t)ptr - (size_t)&buffer[h->ridx];
}

ssize_t buf_handle_offset(int handle_id)
{
    const struct memory_handle *h = find_handle(handle_id);
    if (!h)
        return ERR_HANDLE_NOT_FOUND;
    return h->offset;
}

void buf_request_buffer_handle(int handle_id)
{
    LOGFQUEUE("buffering >| Q_START_FILL %d",handle_id);
    queue_send(&buffering_queue, Q_START_FILL, handle_id);
}

void buf_set_base_handle(int handle_id)
{
    LOGFQUEUE("buffering > Q_BASE_HANDLE %d", handle_id);
    queue_post(&buffering_queue, Q_BASE_HANDLE, handle_id);
}

/* Return the amount of buffer space used */
size_t buf_used(void)
{
    return BUF_USED;
}

void buf_set_watermark(size_t bytes)
{
    conf_watermark = bytes;
}

static void shrink_buffer_inner(struct memory_handle *h)
{
    if (h == NULL)
        return;

    shrink_buffer_inner(h->next);

    shrink_handle(h);
}

static void shrink_buffer(void)
{
    logf("shrink_buffer()");
    shrink_buffer_inner(first_handle);
}

void buffering_thread(void)
{
    bool filling = false;
    struct queue_event ev;

    while (true)
    {
        if (!filling) {
            cancel_cpu_boost();
        }

        queue_wait_w_tmo(&buffering_queue, &ev, filling ? 5 : HZ/2);

        switch (ev.id)
        {
            case Q_START_FILL:
                LOGFQUEUE("buffering < Q_START_FILL %d", (int)ev.data);
                /* Call buffer callbacks here because this is one of two ways
                 * to begin a full buffer fill */
                send_event(BUFFER_EVENT_BUFFER_LOW, 0);
                shrink_buffer();
                queue_reply(&buffering_queue, 1);
                filling |= buffer_handle((int)ev.data);
                break;

            case Q_BUFFER_HANDLE:
                LOGFQUEUE("buffering < Q_BUFFER_HANDLE %d", (int)ev.data);
                queue_reply(&buffering_queue, 1);
                buffer_handle((int)ev.data);
                break;

            case Q_RESET_HANDLE:
                LOGFQUEUE("buffering < Q_RESET_HANDLE %d", (int)ev.data);
                queue_reply(&buffering_queue, 1);
                reset_handle((int)ev.data);
                break;

            case Q_CLOSE_HANDLE:
                LOGFQUEUE("buffering < Q_CLOSE_HANDLE %d", (int)ev.data);
                queue_reply(&buffering_queue, close_handle((int)ev.data));
                break;

            case Q_HANDLE_ADDED:
                LOGFQUEUE("buffering < Q_HANDLE_ADDED %d", (int)ev.data);
                /* A handle was added: the disk is spinning, so we can fill */
                filling = true;
                break;

            case Q_BASE_HANDLE:
                LOGFQUEUE("buffering < Q_BASE_HANDLE %d", (int)ev.data);
                base_handle_id = (int)ev.data;
                break;

#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                LOGFQUEUE("buffering < SYS_USB_CONNECTED");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&buffering_queue);
                break;
#endif

            case SYS_TIMEOUT:
                LOGFQUEUE_SYS_TIMEOUT("buffering < SYS_TIMEOUT");
                break;
        }

        update_data_counters();

        /* If the buffer is low, call the callbacks to get new data */
        if (num_handles > 0 && data_counters.useful <= conf_watermark)
            send_event(BUFFER_EVENT_BUFFER_LOW, 0);

#if 0
        /* TODO: This needs to be fixed to use the idle callback, disable it
         * for simplicity until its done right */
#if MEM > 8
        /* If the disk is spinning, take advantage by filling the buffer */
        else if (storage_disk_is_active() && queue_empty(&buffering_queue))
        {
            if (num_handles > 0 && data_counters.useful <= high_watermark)
                send_event(BUFFER_EVENT_BUFFER_LOW, 0);

            if (data_counters.remaining > 0 && BUF_USED <= high_watermark)
            {
                /* This is a new fill, shrink the buffer up first */
                if (!filling)
                    shrink_buffer();
                filling = fill_buffer();
                update_data_counters();
            }
        }
#endif
#endif

        if (queue_empty(&buffering_queue)) {
            if (filling) {
                if (data_counters.remaining > 0 && BUF_USED < buffer_len)
                    filling = fill_buffer();
                else if (data_counters.remaining == 0)
                    filling = false;
            }
            else if (ev.id == SYS_TIMEOUT)
            {
                if (data_counters.remaining > 0 &&
                    data_counters.useful <= conf_watermark) {
                    shrink_buffer();
                    filling = fill_buffer();
                }
            }
        }
    }
}

void buffering_init(void)
{
    mutex_init(&llist_mutex);
    mutex_init(&llist_mod_mutex);
#ifdef HAVE_PRIORITY_SCHEDULING
    /* This behavior not safe atm */
    mutex_set_preempt(&llist_mutex, false);
    mutex_set_preempt(&llist_mod_mutex, false);
#endif

    conf_watermark = BUFFERING_DEFAULT_WATERMARK;

    queue_init(&buffering_queue, true);
    buffering_thread_id = create_thread( buffering_thread, buffering_stack,
            sizeof(buffering_stack), CREATE_THREAD_FROZEN,
            buffering_thread_name IF_PRIO(, PRIORITY_BUFFERING)
            IF_COP(, CPU));

    queue_enable_queue_send(&buffering_queue, &buffering_queue_sender_list,
                            buffering_thread_id);
}

/* Initialise the buffering subsystem */
bool buffering_reset(char *buf, size_t buflen)
{
    if (!buf || !buflen)
        return false;

    buffer = buf;
    /* Preserve alignment when wrapping around */
    buffer_len = buflen & ~STORAGE_ALIGN_MASK;
    guard_buffer = buf + buflen;

    buf_widx = 0;
    buf_ridx = 0;

    first_handle = NULL;
    cur_handle = NULL;
    cached_handle = NULL;
    num_handles = 0;
    base_handle_id = -1;

    /* Set the high watermark as 75% full...or 25% empty :) */
#if MEM > 8
    high_watermark = 3*buflen / 4;
#endif

    thread_thaw(buffering_thread_id);

    return true;
}

void buffering_get_debugdata(struct buffering_debug *dbgdata)
{
    update_data_counters();
    dbgdata->num_handles = num_handles;
    dbgdata->data_rem = data_counters.remaining;
    dbgdata->wasted_space = data_counters.wasted;
    dbgdata->buffered_data = data_counters.buffered;
    dbgdata->useful_data = data_counters.useful;
    dbgdata->watermark = conf_watermark;
}
