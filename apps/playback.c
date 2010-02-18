/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005-2007 Miika Pekkarinen
 * Copyright (C) 2007-2008 Nicolas Pennequin
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

/* TODO: Pause should be handled in here, rather than PCMBUF so that voice can
 * play whilst audio is paused */

#include <string.h>
#include "playback.h"
#include "codec_thread.h"
#include "kernel.h"
#include "codecs.h"
#include "buffering.h"
#include "voice_thread.h"
#include "usb.h"
#include "ata.h"
#include "playlist.h"
#include "pcmbuf.h"
#include "buffer.h"
#include "cuesheet.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif
#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_ALBUMART
#include "albumart.h"
#endif
#endif
#include "sound.h"
#include "metadata.h"
#include "splash.h"
#include "talk.h"

#ifdef HAVE_RECORDING
#include "pcm_record.h"
#endif

#define PLAYBACK_VOICE

/* amount of guess-space to allow for codecs that must hunt and peck
 * for their correct seeek target, 32k seems a good size */
#define AUDIO_REBUFFER_GUESS_SIZE    (1024*32)

/* Define LOGF_ENABLE to enable logf output in this file */
/*#define LOGF_ENABLE*/
#include "logf.h"

/* macros to enable logf for queues
   logging on SYS_TIMEOUT can be disabled */
#ifdef SIMULATOR
/* Define this for logf output of all queuing except SYS_TIMEOUT */
#define PLAYBACK_LOGQUEUES
/* Define this to logf SYS_TIMEOUT messages */
/*#define PLAYBACK_LOGQUEUES_SYS_TIMEOUT*/
#endif

#ifdef PLAYBACK_LOGQUEUES
#define LOGFQUEUE logf
#else
#define LOGFQUEUE(...)
#endif

#ifdef PLAYBACK_LOGQUEUES_SYS_TIMEOUT
#define LOGFQUEUE_SYS_TIMEOUT logf
#else
#define LOGFQUEUE_SYS_TIMEOUT(...)
#endif


static enum filling_state {
    STATE_IDLE,     /* audio is stopped: nothing to do */
    STATE_FILLING,  /* adding tracks to the buffer */
    STATE_FULL,     /* can't add any more tracks */
    STATE_END_OF_PLAYLIST, /* all remaining tracks have been added */
    STATE_FINISHED, /* all remaining tracks are fully buffered */
} filling;

/* As defined in plugins/lib/xxx2wav.h */
#define GUARD_BUFSIZE  (32*1024)

bool audio_is_initialized = false;
static bool audio_thread_ready SHAREDBSS_ATTR = false;

/* Variables are commented with the threads that use them: *
 * A=audio, C=codec, V=voice. A suffix of - indicates that *
 * the variable is read but not updated on that thread.    */
/* TBD: Split out "audio" and "playback" (ie. calling) threads */

/* Main state control */
static volatile bool playing SHAREDBSS_ATTR = false;/* Is audio playing? (A) */
static volatile bool paused SHAREDBSS_ATTR = false; /* Is audio paused? (A/C-) */
extern volatile bool audio_codec_loaded;            /* Codec loaded? (C/A-) */

/* Ring buffer where compressed audio and codecs are loaded */
static unsigned char *filebuf = NULL;       /* Start of buffer (A/C-) */
static unsigned char *malloc_buf = NULL;    /* Start of malloc buffer (A/C-) */
static size_t filebuflen = 0;               /* Size of buffer (A/C-) */
/* FIXME: make buf_ridx (C/A-) */

/* Possible arrangements of the buffer */
enum audio_buffer_state
{
    AUDIOBUF_STATE_TRASHED = -1,    /* trashed; must be reset */
    AUDIOBUF_STATE_INITIALIZED = 0, /* voice+audio OR audio-only */
    AUDIOBUF_STATE_VOICED_ONLY = 1, /* voice-only */
};
static int buffer_state = AUDIOBUF_STATE_TRASHED; /* Buffer state */

/* These are used to store the current and next (or prev if the current is the last)
 * mp3entry's in a round-robin system. This guarentees that the pointer returned
 * by audio_current/next_track will be valid for the full duration of the
 * currently playing track */
static struct mp3entry mp3entry_buf[2];
struct mp3entry *thistrack_id3,  /* the currently playing track */
                *othertrack_id3; /* prev track during track-change-transition, or end of playlist,
                                  * next track otherwise */
static struct mp3entry unbuffered_id3; /* the id3 for the first unbuffered track */

/* for cuesheet support */
static struct cuesheet *curr_cue = NULL;


#define MAX_MULTIPLE_AA 2

#ifdef HAVE_ALBUMART
static struct albumart_slot {
    struct dim dim;     /* holds width, height of the albumart */
    int used;           /* counter, increments if something uses it */
} albumart_slots[MAX_MULTIPLE_AA];

#define FOREACH_ALBUMART(i) for(i = 0;i < MAX_MULTIPLE_AA; i++)
#endif


#define MAX_TRACK       128
#define MAX_TRACK_MASK  (MAX_TRACK-1)

/* Track info structure about songs in the file buffer (A/C-) */
static struct track_info {
    int audio_hid;             /* The ID for the track's buffer handle */
    int id3_hid;               /* The ID for the track's metadata handle */
    int codec_hid;             /* The ID for the track's codec handle */
#ifdef HAVE_ALBUMART
    int aa_hid[MAX_MULTIPLE_AA];/* The ID for the track's album art handle */
#endif
    int cuesheet_hid;          /* The ID for the track's parsed cueesheet handle */

    size_t filesize;           /* File total length */

    bool taginfo_ready;        /* Is metadata read */
    
} tracks[MAX_TRACK];

static volatile int track_ridx = 0;  /* Track being decoded (A/C-) */
static int track_widx = 0;           /* Track being buffered (A) */
#define CUR_TI (&tracks[track_ridx]) /* Playing track info pointer (A/C-) */

static struct track_info *prev_ti = NULL;  /* Pointer to the previously played
                                              track */

/* Information used only for filling the buffer */
/* Playlist steps from playing track to next track to be buffered (A) */
static int last_peek_offset = 0;

/* Scrobbler support */
static unsigned long prev_track_elapsed = 0; /* Previous track elapsed time (C/A-)*/

/* Track change controls */
bool automatic_skip = false;        /* Who initiated in-progress skip? (C/A-) */
extern bool track_transition;       /* Are we in a track transition? */
static bool dir_skip = false;       /* Is a directory skip pending? (A) */
static bool new_playlist = false;   /* Are we starting a new playlist? (A) */
static int wps_offset = 0;          /* Pending track change offset, to keep WPS responsive (A) */
static bool skipped_during_pause = false; /* Do we need to clear the PCM buffer when playback resumes (A) */

static bool start_play_g = false; /* Used by audio_load_track to notify
                                     audio_finish_load_track about start_play */

/* True when a track load is in progress, i.e. audio_load_track() has returned
 * but audio_finish_load_track() hasn't been called yet. Used to avoid allowing
 * audio_load_track() to get called twice in a row, which would cause problems.
 */
static bool track_load_started = false;

#ifdef HAVE_DISK_STORAGE
static size_t buffer_margin  = 5; /* Buffer margin aka anti-skip buffer (A/C-) */
#endif

/* Event queues */
struct event_queue audio_queue SHAREDBSS_ATTR;
struct event_queue codec_queue SHAREDBSS_ATTR;
static struct event_queue pcmbuf_queue SHAREDBSS_ATTR;

extern struct codec_api ci;
extern unsigned int codec_thread_id;

/* Multiple threads */
/* Set the watermark to trigger buffer fill (A/C) */
static void set_filebuf_watermark(void);

/* Audio thread */
static struct queue_sender_list audio_queue_sender_list SHAREDBSS_ATTR;
static long audio_stack[(DEFAULT_STACK_SIZE + 0x1000)/sizeof(long)];
static const char audio_thread_name[] = "audio";

static void audio_thread(void);
static void audio_initiate_track_change(long direction);
static bool audio_have_tracks(void);
static void audio_reset_buffer(void);
static void audio_stop_playback(void);


/**************************************/


/** Pcmbuf callbacks */

/* Between the codec and PCM track change, we need to keep updating the
 * "elapsed" value of the previous (to the codec, but current to the
 * user/PCM/WPS) track, so that the progressbar reaches the end.
 * During that transition, the WPS will display othertrack_id3. */
void audio_pcmbuf_position_callback(unsigned int time)
{
    time += othertrack_id3->elapsed;

    if (time >= othertrack_id3->length)
    {
        /* we just played the end of the track, so stop this callback */
        track_transition = false;
        othertrack_id3->elapsed = othertrack_id3->length;
    }
    else
        othertrack_id3->elapsed = time;
}

/* Post message from pcmbuf that the end of the previous track
 * has just been played. */
void audio_post_track_change(bool pcmbuf)
{
    if (pcmbuf)
    {
        LOGFQUEUE("pcmbuf > pcmbuf Q_AUDIO_TRACK_CHANGED");
        queue_post(&pcmbuf_queue, Q_AUDIO_TRACK_CHANGED, 0);
    }
    else
    {
        LOGFQUEUE("pcmbuf > audio Q_AUDIO_TRACK_CHANGED");
        queue_post(&audio_queue, Q_AUDIO_TRACK_CHANGED, 0);
    }
}

/* Scan the pcmbuf queue and return true if a message pulled */
static bool pcmbuf_queue_scan(struct queue_event *ev)
{
    if (!queue_empty(&pcmbuf_queue))
    {
        /* Transfer message to audio queue */
        pcm_play_lock();
        /* Pull message - never, ever any blocking call! */
        queue_wait_w_tmo(&pcmbuf_queue, ev, 0);
        pcm_play_unlock();
        return true;
    }

    return false;
}


/** Helper functions */

static struct mp3entry *bufgetid3(int handle_id)
{
    if (handle_id < 0)
        return NULL;

    struct mp3entry *id3;
    ssize_t ret = bufgetdata(handle_id, 0, (void *)&id3);

    if (ret < 0 || ret != sizeof(struct mp3entry))
        return NULL;

    return id3;
}

static bool clear_track_info(struct track_info *track)
{
    /* bufclose returns true if the handle is not found, or if it is closed
     * successfully, so these checks are safe on non-existant handles */
    if (!track)
        return false;

    if (track->codec_hid >= 0) {
        if (bufclose(track->codec_hid))
            track->codec_hid = -1;
        else
            return false;
    }

    if (track->id3_hid >= 0) {
        if (bufclose(track->id3_hid))
            track->id3_hid = -1;
        else
            return false;
    }

    if (track->audio_hid >= 0) {
        if (bufclose(track->audio_hid))
            track->audio_hid = -1;
        else
            return false;
    }

#ifdef HAVE_ALBUMART
    {
        int i;
        FOREACH_ALBUMART(i)
        {
            if (track->aa_hid[i] >= 0) {
                if (bufclose(track->aa_hid[i]))
                    track->aa_hid[i] = -1;
                else
                    return false;
            }   
        }
    }
#endif

    if (track->cuesheet_hid >= 0) {
        if (bufclose(track->cuesheet_hid))
            track->cuesheet_hid = -1;
        else
            return false;
    }

    track->filesize = 0;
    track->taginfo_ready = false;

    return true;
}

/* --- External interfaces --- */

/* This sends a stop message and the audio thread will dump all it's
   subsequenct messages */
void audio_hard_stop(void)
{
    /* Stop playback */
    LOGFQUEUE("audio >| audio Q_AUDIO_STOP: 1");
    queue_send(&audio_queue, Q_AUDIO_STOP, 1);
#ifdef PLAYBACK_VOICE
    voice_stop();
#endif
}

bool audio_restore_playback(int type)
{
    switch (type)
    {
    case AUDIO_WANT_PLAYBACK:
        if (buffer_state != AUDIOBUF_STATE_INITIALIZED)
            audio_reset_buffer();
        return true;
    case AUDIO_WANT_VOICE:
        if (buffer_state == AUDIOBUF_STATE_TRASHED)
            audio_reset_buffer();
        return true;
    default:
        return false;
    }
}

unsigned char *audio_get_buffer(bool talk_buf, size_t *buffer_size)
{
    unsigned char *buf, *end;

    if (audio_is_initialized)
    {
        audio_hard_stop();
    }
    /* else buffer_state will be AUDIOBUF_STATE_TRASHED at this point */

    /* Reset the buffering thread so that it doesn't try to use the data */
    buffering_reset(filebuf, filebuflen);

    if (buffer_size == NULL)
    {
        /* Special case for talk_init to use since it already knows it's
           trashed */
        buffer_state = AUDIOBUF_STATE_TRASHED;
        return NULL;
    }

    if (talk_buf || buffer_state == AUDIOBUF_STATE_TRASHED
           || !talk_voice_required())
    {
        logf("get buffer: talk, audio");
        /* Ok to use everything from audiobuf to audiobufend - voice is loaded,
           the talk buffer is not needed because voice isn't being used, or
           could be AUDIOBUF_STATE_TRASHED already. If state is
           AUDIOBUF_STATE_VOICED_ONLY, no problem as long as memory isn't written
           without the caller knowing what's going on. Changing certain settings
           may move it to a worse condition but the memory in use by something
           else will remain undisturbed.
         */
        if (buffer_state != AUDIOBUF_STATE_TRASHED)
        {
            talk_buffer_steal();
            buffer_state = AUDIOBUF_STATE_TRASHED;
        }

        buf = audiobuf;
        end = audiobufend;
    }
    else
    {
        /* Safe to just return this if already AUDIOBUF_STATE_VOICED_ONLY or
           still AUDIOBUF_STATE_INITIALIZED */
        /* Skip talk buffer and move pcm buffer to end to maximize available
           contiguous memory - no audio running means voice will not need the
           swap space */
        logf("get buffer: audio");
        buf = audiobuf + talk_get_bufsize();
        end = audiobufend - pcmbuf_init(audiobufend);
        buffer_state = AUDIOBUF_STATE_VOICED_ONLY;
    }

    *buffer_size = end - buf;

    return buf;
}

bool audio_buffer_state_trashed(void)
{
    return buffer_state == AUDIOBUF_STATE_TRASHED;
}

#ifdef HAVE_RECORDING
unsigned char *audio_get_recording_buffer(size_t *buffer_size)
{
    /* Stop audio, voice and obtain all available buffer space */
    audio_hard_stop();
    talk_buffer_steal();

    unsigned char *end = audiobufend;
    buffer_state = AUDIOBUF_STATE_TRASHED;
    *buffer_size = end - audiobuf;

    return (unsigned char *)audiobuf;
}

bool audio_load_encoder(int afmt)
{
#ifndef SIMULATOR
    const char *enc_fn = get_codec_filename(afmt | CODEC_TYPE_ENCODER);
    if (!enc_fn)
        return false;

    audio_remove_encoder();
    ci.enc_codec_loaded = 0; /* clear any previous error condition */

    LOGFQUEUE("codec > Q_ENCODER_LOAD_DISK");
    queue_post(&codec_queue, Q_ENCODER_LOAD_DISK, (intptr_t)enc_fn);

    while (ci.enc_codec_loaded == 0)
        yield();

    logf("codec loaded: %d", ci.enc_codec_loaded);

    return ci.enc_codec_loaded > 0;
#else
    (void)afmt;
    return true;
#endif
} /* audio_load_encoder */

void audio_remove_encoder(void)
{
#ifndef SIMULATOR
    /* force encoder codec unload (if currently loaded) */
    if (ci.enc_codec_loaded <= 0)
        return;

    ci.stop_encoder = true;
    while (ci.enc_codec_loaded > 0)
        yield();
#endif
} /* audio_remove_encoder */

#endif /* HAVE_RECORDING */


struct mp3entry* audio_current_track(void)
{
    const char *filename;
    struct playlist_track_info trackinfo;
    int cur_idx;
    int offset = ci.new_track + wps_offset;
    struct mp3entry *write_id3;

    cur_idx = (track_ridx + offset) & MAX_TRACK_MASK;

    if (cur_idx == track_ridx && *thistrack_id3->path)
    {
        /* The usual case */
        if (tracks[cur_idx].cuesheet_hid >= 0 && !thistrack_id3->cuesheet)
        {
            bufread(tracks[cur_idx].cuesheet_hid, sizeof(struct cuesheet), curr_cue);
            thistrack_id3->cuesheet = curr_cue;
            cue_spoof_id3(thistrack_id3->cuesheet, thistrack_id3);
        }
        return thistrack_id3;
    }
    else if (automatic_skip && offset == -1 && *othertrack_id3->path)
    {
        /* We're in a track transition. The codec has moved on to the next track,
             but the audio being played is still the same (now previous) track.
             othertrack_id3.elapsed is being updated in an ISR by
             codec_pcmbuf_position_callback */
        if (tracks[cur_idx].cuesheet_hid >= 0 && !thistrack_id3->cuesheet)
        {
            bufread(tracks[cur_idx].cuesheet_hid, sizeof(struct cuesheet), curr_cue);
            othertrack_id3->cuesheet = curr_cue;
            cue_spoof_id3(othertrack_id3->cuesheet, othertrack_id3);
        }
        return othertrack_id3;
    }

    if (offset != 0)
    {
        /* Codec may be using thistrack_id3, so it must not be overwritten.
             If this is a manual skip, othertrack_id3 will become 
             thistrack_id3 in audio_check_new_track().
           FIXME: If this is an automatic skip, it probably means multiple 
             short tracks fit in the PCM buffer.  Overwriting othertrack_id3
             can lead to an incorrect value later.
           Note that othertrack_id3 may also be used for next track.
          */
        write_id3 = othertrack_id3;
    }
    else 
    {
        write_id3 = thistrack_id3;
    }

    if (tracks[cur_idx].id3_hid >= 0)
    {
        /* The current track's info has been buffered but not read yet, so get it */
        if (bufread(tracks[cur_idx].id3_hid, sizeof(struct mp3entry), write_id3)
             == sizeof(struct mp3entry))
            return write_id3;
    }

    /* We didn't find the ID3 metadata, so we fill temp_id3 with the little info
       we have and return that. */

    memset(write_id3, 0, sizeof(struct mp3entry));

    playlist_get_track_info(NULL, playlist_next(0)+wps_offset, &trackinfo);
    filename = trackinfo.filename;
    if (!filename)
        filename = "No file!";

#if defined(HAVE_TC_RAMCACHE) && defined(HAVE_DIRCACHE)
    if (tagcache_fill_tags(write_id3, filename))
        return write_id3;
#endif

    strlcpy(write_id3->path, filename, sizeof(write_id3->path));
    write_id3->title = strrchr(write_id3->path, '/');
    if (!write_id3->title)
        write_id3->title = &write_id3->path[0];
    else
        write_id3->title++;

    return write_id3;
}

struct mp3entry* audio_next_track(void)
{
    int next_idx;
    int offset = ci.new_track + wps_offset;

    if (!audio_have_tracks())
        return NULL;

    if (wps_offset == -1 && *thistrack_id3->path)
    {
        /* We're in a track transition. The next track for the WPS is the one
           currently being decoded. */
        return thistrack_id3;
    }

    next_idx = (track_ridx + offset + 1) & MAX_TRACK_MASK;

    if (tracks[next_idx].id3_hid >= 0)
    {
        if (bufread(tracks[next_idx].id3_hid, sizeof(struct mp3entry), othertrack_id3)
             == sizeof(struct mp3entry))
            return othertrack_id3;
        else
            return NULL;
    }

    if (next_idx == track_widx)
    {
        /* The next track hasn't been buffered yet, so we return the static
           version of its metadata. */
        return &unbuffered_id3;
    }

    return NULL;
}

bool audio_peek_track(struct mp3entry* id3, int offset)
{
    int next_idx;
    int new_offset = ci.new_track + wps_offset + offset;

    if (!audio_have_tracks())
        return false;
    next_idx = (track_ridx + new_offset) & MAX_TRACK_MASK;

    if (tracks[next_idx].id3_hid >= 0)
    {
        return bufread(tracks[next_idx].id3_hid, sizeof(struct mp3entry), id3) 
                    == sizeof(struct mp3entry);
    }
    return false;
}

#ifdef HAVE_ALBUMART
int playback_current_aa_hid(int slot)
{
    if (slot < 0)
        return -1;
    int cur_idx;
    int offset = ci.new_track + wps_offset;

    cur_idx = track_ridx + offset;
    cur_idx &= MAX_TRACK_MASK;

    return tracks[cur_idx].aa_hid[slot];
}

int playback_claim_aa_slot(struct dim *dim)
{
    int i;
    /* first try to find a slot already having the size to reuse it
     * since we don't want albumart of the same size buffered multiple times */
    FOREACH_ALBUMART(i)
    {
        struct albumart_slot *slot = &albumart_slots[i];
        if (slot->dim.width == dim->width
                && slot->dim.height == dim->height)
        {
            slot->used++;
            return i;
        }
    }
    /* size is new, find a free slot */
    FOREACH_ALBUMART(i)
    {
        if (!albumart_slots[i].used)
        {
            albumart_slots[i].used++;
            albumart_slots[i].dim = *dim;
            return i;
        }
    }
    /* sorry, no free slot */
    return -1;
}

void playback_release_aa_slot(int slot)
{
    /* invalidate the albumart_slot */
    struct albumart_slot *aa_slot = &albumart_slots[slot];
    if (aa_slot->used > 0)
        aa_slot->used--;
}

#endif
void audio_play(long offset)
{
    logf("audio_play");

#ifdef PLAYBACK_VOICE
    /* Truncate any existing voice output so we don't have spelling
     * etc. over the first part of the played track */
    talk_force_shutup();
#endif

    /* Start playback */
    LOGFQUEUE("audio >| audio Q_AUDIO_PLAY: %ld", offset);
    /* Don't return until playback has actually started */
    queue_send(&audio_queue, Q_AUDIO_PLAY, offset);
}

void audio_stop(void)
{
    /* Stop playback */
    LOGFQUEUE("audio >| audio Q_AUDIO_STOP");
    /* Don't return until playback has actually stopped */
    queue_send(&audio_queue, Q_AUDIO_STOP, 0);
}

void audio_pause(void)
{
    LOGFQUEUE("audio >| audio Q_AUDIO_PAUSE");
    /* Don't return until playback has actually paused */
    queue_send(&audio_queue, Q_AUDIO_PAUSE, true);
}

void audio_resume(void)
{
    LOGFQUEUE("audio >| audio Q_AUDIO_PAUSE resume");
    /* Don't return until playback has actually resumed */
    queue_send(&audio_queue, Q_AUDIO_PAUSE, false);
}

void audio_skip(int direction)
{
    if (playlist_check(ci.new_track + wps_offset + direction))
    {
        if (global_settings.beep)
            pcmbuf_beep(2000, 100, 2500*global_settings.beep);

        LOGFQUEUE("audio > audio Q_AUDIO_SKIP %d", direction);
        queue_post(&audio_queue, Q_AUDIO_SKIP, direction);
        /* Update wps while our message travels inside deep playback queues. */
        wps_offset += direction;
    }
    else
    {
        /* No more tracks. */
        if (global_settings.beep)
            pcmbuf_beep(1000, 100, 1500*global_settings.beep);
    }
}

void audio_next(void)
{
    audio_skip(1);
}

void audio_prev(void)
{
    audio_skip(-1);
}

void audio_next_dir(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_DIR_SKIP 1");
    queue_post(&audio_queue, Q_AUDIO_DIR_SKIP, 1);
}

void audio_prev_dir(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_DIR_SKIP -1");
    queue_post(&audio_queue, Q_AUDIO_DIR_SKIP, -1);
}

void audio_pre_ff_rewind(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_PRE_FF_REWIND");
    queue_post(&audio_queue, Q_AUDIO_PRE_FF_REWIND, 0);
}

void audio_ff_rewind(long newpos)
{
    LOGFQUEUE("audio > audio Q_AUDIO_FF_REWIND");
    queue_post(&audio_queue, Q_AUDIO_FF_REWIND, newpos);
}

void audio_flush_and_reload_tracks(void)
{
    LOGFQUEUE("audio > audio Q_AUDIO_FLUSH");
    queue_post(&audio_queue, Q_AUDIO_FLUSH, 0);
}

void audio_error_clear(void)
{
#ifdef AUDIO_HAVE_RECORDING
    pcm_rec_error_clear();
#endif
}

int audio_status(void)
{
    int ret = 0;

    if (playing)
        ret |= AUDIO_STATUS_PLAY;

    if (paused)
        ret |= AUDIO_STATUS_PAUSE;

#ifdef HAVE_RECORDING
    /* Do this here for constitency with mpeg.c version */
    /* FIXME: pcm_rec_status() is deprecated */
    ret |= pcm_rec_status();
#endif

    return ret;
}

int audio_get_file_pos(void)
{
    return 0;
}

#ifdef HAVE_DISK_STORAGE
void audio_set_buffer_margin(int setting)
{
    static const unsigned short lookup[] = {5, 15, 30, 60, 120, 180, 300, 600};
    buffer_margin = lookup[setting];
    logf("buffer margin: %ld", (long)buffer_margin);
    set_filebuf_watermark();
}
#endif

#ifdef HAVE_CROSSFADE
/* Take necessary steps to enable or disable the crossfade setting */
void audio_set_crossfade(int enable)
{
    size_t offset;
    bool was_playing;
    size_t size;

    /* Tell it the next setting to use */
    pcmbuf_request_crossfade_enable(enable);

    /* Return if size hasn't changed or this is too early to determine
       which in the second case there's no way we could be playing
       anything at all */
    if (pcmbuf_is_same_size()) return;

    offset = 0;
    was_playing = playing;

    /* Playback has to be stopped before changing the buffer size */
    if (was_playing)
    {
        /* Store the track resume position */
        offset = thistrack_id3->offset;
    }

    /* Blast it - audio buffer will have to be setup again next time
       something plays */
    audio_get_buffer(true, &size);

    /* Restart playback if audio was running previously */
    if (was_playing)
        audio_play(offset);
}
#endif

/* --- Routines called from multiple threads --- */

static void set_filebuf_watermark(void)
{
    if (!filebuf)
        return;     /* Audio buffers not yet set up */

#ifdef HAVE_DISK_STORAGE
    int seconds;
    int spinup = ata_spinup_time();
    if (spinup)
        seconds = (spinup / HZ) + 1;
    else
        seconds = 5;

    seconds += buffer_margin;
#else
    /* flash storage */
    int seconds = 1;
#endif

    /* bitrate of last track in buffer dictates watermark */
    struct mp3entry* id3 = NULL;
    if (tracks[track_widx].taginfo_ready)
        id3 = bufgetid3(tracks[track_widx].id3_hid);
    else
        id3 = bufgetid3(tracks[track_widx-1].id3_hid);
    if (!id3) {
        logf("fwmark: No id3 for last track (r%d/w%d), aborting!", track_ridx, track_widx);
        return;
    }
    size_t bytes = id3->bitrate * (1000/8) * seconds;
    buf_set_watermark(bytes);
    logf("fwmark: %d", bytes);
}

/* --- Buffering callbacks --- */

static void buffering_low_buffer_callback(void *data)
{
    (void)data;
    logf("low buffer callback");

    if (filling == STATE_FULL || filling == STATE_END_OF_PLAYLIST) {
        /* force a refill */
        LOGFQUEUE("buffering > audio Q_AUDIO_FILL_BUFFER");
        queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);
    }
}

static void buffering_handle_rebuffer_callback(void *data)
{
    (void)data;
    LOGFQUEUE("audio >| audio Q_AUDIO_FLUSH");
    queue_post(&audio_queue, Q_AUDIO_FLUSH, 0);
}

static void buffering_handle_finished_callback(void *data)
{
    logf("handle %d finished buffering", *(int*)data);
    int hid = (*(int*)data);

    if (hid == tracks[track_widx].id3_hid)
    {
        int offset = ci.new_track + wps_offset;
        int next_idx = (track_ridx + offset + 1) & MAX_TRACK_MASK;
        /* The metadata handle for the last loaded track has been buffered.
           We can ask the audio thread to load the rest of the track's data. */
        LOGFQUEUE("audio >| audio Q_AUDIO_FINISH_LOAD");
        queue_post(&audio_queue, Q_AUDIO_FINISH_LOAD, 0);
        if (tracks[next_idx].id3_hid == hid)
            send_event(PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE, NULL);
    }
    else
    {
        /* This is most likely an audio handle, so we strip the useless
           trailing tags that are left. */
        strip_tags(hid);

        if (hid == tracks[track_widx-1].audio_hid
            && filling == STATE_END_OF_PLAYLIST)
        {
            /* This was the last track in the playlist.
               We now have all the data we need. */
            logf("last track finished buffering");
            filling = STATE_FINISHED;
        }
    }
}


/* --- Audio thread --- */

static bool audio_have_tracks(void)
{
    return (audio_track_count() != 0);
}

static int audio_free_track_count(void)
{
    /* Used tracks + free tracks adds up to MAX_TRACK - 1 */
    return MAX_TRACK - 1 - audio_track_count();
}

int audio_track_count(void)
{
    /* Calculate difference from track_ridx to track_widx
     * taking into account a possible wrap-around. */
    return (MAX_TRACK + track_widx - track_ridx) & MAX_TRACK_MASK;
}

long audio_filebufused(void)
{
    return (long) buf_used();
}

/* Update track info after successful a codec track change */
static void audio_update_trackinfo(void)
{
    /* Load the curent track's metadata into curtrack_id3 */
    if (CUR_TI->id3_hid >= 0)
        copy_mp3entry(thistrack_id3, bufgetid3(CUR_TI->id3_hid));

    /* Reset current position */
    thistrack_id3->elapsed = 0;
    thistrack_id3->offset = 0;

    /* Update the codec API */
    ci.filesize = CUR_TI->filesize;
    ci.id3 = thistrack_id3;
    ci.curpos = 0;
    ci.taginfo_ready = &CUR_TI->taginfo_ready;
}

/* Clear tracks between write and read, non inclusive */
static void audio_clear_track_entries(void)
{
    int cur_idx = track_widx;

    logf("Clearing tracks: r%d/w%d", track_ridx, track_widx);

    /* Loop over all tracks from write-to-read */
    while (1)
    {
        cur_idx = (cur_idx + 1) & MAX_TRACK_MASK;

        if (cur_idx == track_ridx)
            break;

        clear_track_info(&tracks[cur_idx]);
    }
}

/* Clear all tracks */
static bool audio_release_tracks(void)
{
    int i, cur_idx;

    logf("releasing all tracks");

    for(i = 0; i < MAX_TRACK; i++)
    {
        cur_idx = (track_ridx + i) & MAX_TRACK_MASK;
        if (!clear_track_info(&tracks[cur_idx]))
            return false;
    }

    return true;
}

static bool audio_loadcodec(bool start_play)
{
    int prev_track;
    char codec_path[MAX_PATH]; /* Full path to codec */
    const struct mp3entry *id3, *prev_id3;

    if (tracks[track_widx].id3_hid < 0) {
        return false;
    }

    id3 = bufgetid3(tracks[track_widx].id3_hid);
    if (!id3)
        return false;

    const char *codec_fn = get_codec_filename(id3->codectype);
    if (codec_fn == NULL)
        return false;

    tracks[track_widx].codec_hid = -1;

    if (start_play)
    {
        /* Load the codec directly from disk and save some memory. */
        track_ridx = track_widx;
        ci.filesize = CUR_TI->filesize;
        ci.id3 = thistrack_id3;
        ci.taginfo_ready = &CUR_TI->taginfo_ready;
        ci.curpos = 0;
        LOGFQUEUE("codec > codec Q_CODEC_LOAD_DISK");
        queue_post(&codec_queue, Q_CODEC_LOAD_DISK, (intptr_t)codec_fn);
        return true;
    }
    else
    {
        /* If we already have another track than this one buffered */
        if (track_widx != track_ridx)
        {
            prev_track = (track_widx - 1) & MAX_TRACK_MASK;

            id3 = bufgetid3(tracks[track_widx].id3_hid);
            prev_id3 = bufgetid3(tracks[prev_track].id3_hid);

            /* If the previous codec is the same as this one, there is no need
             * to put another copy of it on the file buffer */
            if (id3 && prev_id3 &&
                get_codec_base_type(id3->codectype) ==
                get_codec_base_type(prev_id3->codectype)
                && audio_codec_loaded)
            {
                logf("Reusing prev. codec");
                return true;
            }
        }
    }

    codec_get_full_path(codec_path, codec_fn);

    tracks[track_widx].codec_hid = bufopen(codec_path, 0, TYPE_CODEC, NULL);
    if (tracks[track_widx].codec_hid < 0)
        return false;

    logf("Loaded codec");

    return true;
}

/* Load metadata for the next track (with bufopen). The rest of the track
   loading will be handled by audio_finish_load_track once the metadata has been
   actually loaded by the buffering thread. */
static bool audio_load_track(size_t offset, bool start_play)
{
    const char *trackname;
    int fd = -1;

    if (track_load_started) {
        /* There is already a track load in progress, so track_widx hasn't been
           incremented yet. Loading another track would overwrite the one that
           hasn't finished loading. */
        logf("audio_load_track(): a track load is already in progress");
        return false;
    }

    start_play_g = start_play;  /* will be read by audio_finish_load_track */

    /* Stop buffer filling if there is no free track entries.
       Don't fill up the last track entry (we wan't to store next track
       metadata there). */
    if (!audio_free_track_count())
    {
        logf("No free tracks");
        return false;
    }

    last_peek_offset++;
    tracks[track_widx].taginfo_ready = false;

    logf("Buffering track: r%d/w%d", track_ridx, track_widx);
    /* Get track name from current playlist read position. */
    while ((trackname = playlist_peek(last_peek_offset)) != NULL)
    {
        /* Handle broken playlists. */
        fd = open(trackname, O_RDONLY);
        if (fd < 0)
        {
            logf("Open failed");
            /* Skip invalid entry from playlist. */
            playlist_skip_entry(NULL, last_peek_offset);
        }
        else
            break;
    }

    if (!trackname)
    {
        logf("End-of-playlist");
        memset(&unbuffered_id3, 0, sizeof(struct mp3entry));
        filling = STATE_END_OF_PLAYLIST;

        if (thistrack_id3->length == 0 && thistrack_id3->filesize == 0)
        {
            /* Stop playback if no valid track was found. */
            audio_stop_playback();
        }

        return false;
    }

    tracks[track_widx].filesize = filesize(fd);

    if (offset > tracks[track_widx].filesize)
        offset = 0;

    /* Set default values */
    if (start_play)
    {
        buf_set_watermark(filebuflen/2);
        dsp_configure(ci.dsp, DSP_RESET, 0);
        playlist_update_resume_info(audio_current_track());
    }

    /* Get track metadata if we don't already have it. */
    if (tracks[track_widx].id3_hid < 0)
    {
        tracks[track_widx].id3_hid = bufopen(trackname, 0, TYPE_ID3, NULL);

        if (tracks[track_widx].id3_hid < 0)
        {
            /* Buffer is full. */
            get_metadata(&unbuffered_id3, fd, trackname);
            last_peek_offset--;
            close(fd);
            logf("buffer is full for now (get metadata)");
            filling = STATE_FULL;
            return false;
        }

        if (track_widx == track_ridx)
        {
            /* TODO: Superfluos buffering call? */
            buf_request_buffer_handle(tracks[track_widx].id3_hid);
            struct mp3entry *id3 = bufgetid3(tracks[track_widx].id3_hid);
            if (id3)
            {    
                copy_mp3entry(thistrack_id3, id3);
                thistrack_id3->offset = offset;
            }
            else
                memset(thistrack_id3, 0, sizeof(struct mp3entry));
        }

        if (start_play)
        {
            playlist_update_resume_info(audio_current_track());
        }
    }

    close(fd);
    track_load_started = true; /* Remember that we've started loading a track */
    return true;
}

/* Second part of the track loading: We now have the metadata available, so we
   can load the codec, the album art and finally the audio data.
   This is called on the audio thread after the buffering thread calls the
   buffering_handle_finished_callback callback. */
static void audio_finish_load_track(void)
{
    size_t file_offset = 0;
    size_t offset = 0;
    bool start_play = start_play_g;

    track_load_started = false;

    if (tracks[track_widx].id3_hid < 0) {
        logf("No metadata");
        return;
    }

    struct mp3entry *track_id3;

    if (track_widx == track_ridx)
        track_id3 = thistrack_id3;
    else
        track_id3 = bufgetid3(tracks[track_widx].id3_hid);

    if (track_id3->length == 0 && track_id3->filesize == 0)
    {
        logf("audio_finish_load_track: invalid metadata");

        /* Invalid metadata */
        bufclose(tracks[track_widx].id3_hid);
        tracks[track_widx].id3_hid = -1;

        /* Skip invalid entry from playlist. */
        playlist_skip_entry(NULL, last_peek_offset--);

        /* load next track */
        LOGFQUEUE("audio > audio Q_AUDIO_FILL_BUFFER %d", (int)start_play);
        queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, start_play);

        return;
    }
    /* Try to load a cuesheet for the track */
    if (curr_cue)
    {
        char cuepath[MAX_PATH];
        if (look_for_cuesheet_file(track_id3->path, cuepath))
        {
            void *temp;
            tracks[track_widx].cuesheet_hid = 
                        bufalloc(NULL, sizeof(struct cuesheet), TYPE_CUESHEET);
            if (tracks[track_widx].cuesheet_hid >= 0)
            {
                bufgetdata(tracks[track_widx].cuesheet_hid,
                           sizeof(struct cuesheet), &temp);
                struct cuesheet *cuesheet = (struct cuesheet*)temp;
                if (!parse_cuesheet(cuepath, cuesheet))
                {
                    bufclose(tracks[track_widx].cuesheet_hid);
                    track_id3->cuesheet = NULL;
                }
            }
        }
    }
#ifdef HAVE_ALBUMART
    {
        int i;
        char aa_path[MAX_PATH];
        FOREACH_ALBUMART(i)
        {
            /* albumart_slots may change during a yield of bufopen,
             * but that's no problem */
            if (tracks[track_widx].aa_hid[i] >= 0 || !albumart_slots[i].used)
                continue;
            /* find_albumart will error out if the wps doesn't have AA */
            if (find_albumart(track_id3, aa_path, sizeof(aa_path),
                               &(albumart_slots[i].dim)))
            {
                int aa_hid = bufopen(aa_path, 0, TYPE_BITMAP,
                                                &(albumart_slots[i].dim));

                if(aa_hid == ERR_BUFFER_FULL)
                {
                    filling = STATE_FULL;
                    logf("buffer is full for now (get album art)");
                    return;  /* No space for track's album art, not an error */
                }
                else if (aa_hid < 0)
                {
                    /* another error, ignore AlbumArt */
                    logf("Album art loading failed");
                }
                tracks[track_widx].aa_hid[i] = aa_hid;
            }
        }
        
    }
#endif

    /* Load the codec. */
    if (!audio_loadcodec(start_play))
    {
        if (tracks[track_widx].codec_hid == ERR_BUFFER_FULL)
        {
            /* No space for codec on buffer, not an error */
            filling = STATE_FULL;
            return;
        }

        /* This is an error condition, either no codec was found, or reading
         * the codec file failed part way through, either way, skip the track */
        /* FIXME: We should not use splashf from audio thread! */
        splashf(HZ*2, "No codec for: %s", track_id3->path);
        /* Skip invalid entry from playlist. */
        playlist_skip_entry(NULL, last_peek_offset);
        return;
    }

    track_id3->elapsed = 0;
    offset = track_id3->offset;

    enum data_type type = TYPE_PACKET_AUDIO;

    switch (track_id3->codectype) {
    case AFMT_MPA_L1:
    case AFMT_MPA_L2:
    case AFMT_MPA_L3:
        if (offset > 0) {
            file_offset = offset;
            track_id3->offset = offset;
        }
        break;

    case AFMT_WAVPACK:
        if (offset > 0) {
            file_offset = offset;
            track_id3->offset = offset;
            track_id3->elapsed = track_id3->length / 2;
        }
        break;

    case AFMT_OGG_VORBIS:
    case AFMT_SPEEX:
    case AFMT_FLAC:
    case AFMT_PCM_WAV:
    case AFMT_A52:
    case AFMT_MP4_AAC:
    case AFMT_MPC:
    case AFMT_APE:
    case AFMT_WMA:
        if (offset > 0)
            track_id3->offset = offset;
        break;

    case AFMT_NSF:
    case AFMT_SPC:
    case AFMT_SID:
        logf("Loading atomic %d",track_id3->codectype);
        type = TYPE_ATOMIC_AUDIO;
        break;
    }

    logf("load track: %s", track_id3->path);

    if (file_offset > AUDIO_REBUFFER_GUESS_SIZE)
        file_offset -= AUDIO_REBUFFER_GUESS_SIZE;
    else if (track_id3->first_frame_offset)
        file_offset = track_id3->first_frame_offset;
    else
        file_offset = 0;

    tracks[track_widx].audio_hid = bufopen(track_id3->path, file_offset, type,
                                            NULL);

    /* No space left, not an error */
    if (tracks[track_widx].audio_hid == ERR_BUFFER_FULL)
    {
        filling = STATE_FULL;
        logf("buffer is full for now (load audio)");
        return;
    }
    else if (tracks[track_widx].audio_hid < 0)
    {
        /* another error, do not continue either */
        logf("Could not add audio data handle");
        return;
    }

    /* All required data is now available for the codec. */
    tracks[track_widx].taginfo_ready = true;

    if (start_play)
    {
        ci.curpos=file_offset;
        buf_request_buffer_handle(tracks[track_widx].audio_hid);
    }

    track_widx = (track_widx + 1) & MAX_TRACK_MASK;

    send_event(PLAYBACK_EVENT_TRACK_BUFFER, track_id3);

    /* load next track */
    LOGFQUEUE("audio > audio Q_AUDIO_FILL_BUFFER");
    queue_post(&audio_queue, Q_AUDIO_FILL_BUFFER, 0);

    return;
}

static void audio_fill_file_buffer(bool start_play, size_t offset)
{
    trigger_cpu_boost();

    /* No need to rebuffer if there are track skips pending,
     * however don't cancel buffering on skipping while filling. */
    if (ci.new_track != 0 && filling != STATE_FILLING)
        return;
    filling = STATE_FILLING;

    /* Must reset the buffer before use if trashed or voice only - voice
       file size shouldn't have changed so we can go straight from
       AUDIOBUF_STATE_VOICED_ONLY to AUDIOBUF_STATE_INITIALIZED */
    if (buffer_state != AUDIOBUF_STATE_INITIALIZED)
        audio_reset_buffer();

    logf("Starting buffer fill");

    if (!start_play)
        audio_clear_track_entries();

    /* Save the current resume position once. */
    playlist_update_resume_info(audio_current_track());

    audio_load_track(offset, start_play);
}

static void audio_rebuffer(void)
{
    logf("Forcing rebuffer");

    clear_track_info(CUR_TI);

    /* Reset track pointers */
    track_widx = track_ridx;
    audio_clear_track_entries();

    /* Reset a possibly interrupted track load */
    track_load_started = false;

    /* Fill the buffer */
    last_peek_offset = -1;
    ci.curpos = 0;

    if (!CUR_TI->taginfo_ready)
        memset(thistrack_id3, 0, sizeof(struct mp3entry));

    audio_fill_file_buffer(false, 0);
}

/* Called on request from the codec to get a new track. This is the codec part
   of the track transition. */
static int audio_check_new_track(void)
{
    int track_count = audio_track_count();
    int old_track_ridx = track_ridx;
    int i, idx;
    bool forward;
    struct mp3entry *temp = thistrack_id3;

    /* Now it's good time to send track finish events. */
    send_event(PLAYBACK_EVENT_TRACK_FINISH, thistrack_id3);
    /* swap the mp3entry pointers */
    thistrack_id3 = othertrack_id3;
    othertrack_id3 = temp;
    ci.id3 = thistrack_id3;
    memset(thistrack_id3, 0, sizeof(struct mp3entry));
    
    if (dir_skip)
    {
        dir_skip = false;
        /* regardless of the return value we need to rebuffer.
           if it fails the old playlist will resume, else the
           next dir will start playing */
        playlist_next_dir(ci.new_track);
        ci.new_track = 0;
        audio_rebuffer();
        goto skip_done;
    }

    if (new_playlist)
        ci.new_track = 0;

    /* If the playlist isn't that big */
    if (automatic_skip)
    {
        while (!playlist_check(ci.new_track))
        {
            if (ci.new_track >= 0)
            {
                LOGFQUEUE("audio >|= codec Q_CODEC_REQUEST_FAILED");
                return Q_CODEC_REQUEST_FAILED;
            }
            ci.new_track++;
        }
    }

    /* Update the playlist */
    last_peek_offset -= ci.new_track;

    if (playlist_next(ci.new_track) < 0)
    {
        LOGFQUEUE("audio >|= codec Q_CODEC_REQUEST_FAILED");
        return Q_CODEC_REQUEST_FAILED;
    }
    
    if (new_playlist)
    {
        ci.new_track = 1;
        new_playlist = false;
    }

    /* Save a pointer to the old track to allow later clearing */
    prev_ti = CUR_TI;

    for (i = 0; i < ci.new_track; i++)
    {
        idx = (track_ridx + i) & MAX_TRACK_MASK;
        struct mp3entry *id3 = bufgetid3(tracks[idx].id3_hid);
        ssize_t offset = buf_handle_offset(tracks[idx].audio_hid);
        if (!id3 || offset < 0 || (unsigned)offset > id3->first_frame_offset)
        {
            /* We don't have all the audio data for that track, so clear it,
               but keep the metadata. */
            if (tracks[idx].audio_hid >= 0 && bufclose(tracks[idx].audio_hid))
            {
                tracks[idx].audio_hid = -1;
                tracks[idx].filesize = 0;
            }
        }
    }

    /* Move to the new track */
    track_ridx = (track_ridx + ci.new_track) & MAX_TRACK_MASK;


    if (automatic_skip)
    {
        wps_offset = -ci.new_track;
    }

    /* If it is not safe to even skip this many track entries */
    if (ci.new_track >= track_count || ci.new_track <= track_count - MAX_TRACK)
    {
        ci.new_track = 0;
        audio_rebuffer();
        goto skip_done;
    }

    forward = ci.new_track > 0;
    ci.new_track = 0;

    /* If the target track is clearly not in memory */
    if (CUR_TI->filesize == 0 || !CUR_TI->taginfo_ready)
    {
        audio_rebuffer();
        goto skip_done;
    }

    /* When skipping backwards, it is possible that we've found a track that's
     * buffered, but which is around the track-wrap and therefore not the track
     * we are looking for */
    if (!forward)
    {
        int cur_idx = track_ridx;
        bool taginfo_ready = true;
        /* We've wrapped the buffer backwards if new > old */
        bool wrap = track_ridx > old_track_ridx;

        while (1)
        {
            cur_idx = (cur_idx + 1) & MAX_TRACK_MASK;

            /* if we've advanced past the wrap when cur_idx is zeroed */
            if (!cur_idx)
                wrap = false;

            /* if we aren't still on the wrap and we've caught the old track */
            if (!(wrap || cur_idx < old_track_ridx))
                break;

            /* If we hit a track in between without valid tag info, bail */
            if (!tracks[cur_idx].taginfo_ready)
            {
                taginfo_ready = false;
                break;
            }
        }
        if (!taginfo_ready)
        {
            audio_rebuffer();
        }
    }

skip_done:
    audio_update_trackinfo();
    LOGFQUEUE("audio >|= codec Q_CODEC_REQUEST_COMPLETE");
    return Q_CODEC_REQUEST_COMPLETE;
}

unsigned long audio_prev_elapsed(void)
{
    return prev_track_elapsed;
}

void audio_set_prev_elapsed(unsigned long setting)
{
    prev_track_elapsed = setting;
}

static void audio_stop_codec_flush(void)
{
    ci.stop_codec = true;
    pcmbuf_pause(true);

    while (audio_codec_loaded)
        yield();

    /* If the audio codec is not loaded any more, and the audio is still
     * playing, it is now and _only_ now safe to call this function from the
     * audio thread */
    if (pcm_is_playing())
    {
        pcmbuf_play_stop();
        pcm_play_lock();
        queue_clear(&pcmbuf_queue);
        pcm_play_unlock();
    }
    pcmbuf_pause(paused);
}

static void audio_stop_playback(void)
{
    if (playing)
    {
        /* If we were playing, save resume information */
        struct mp3entry *id3 = NULL;

        if (!ci.stop_codec)
        {
            /* Set this early, the outside code yields and may allow the codec
               to try to wait for a reply on a buffer wait */
            ci.stop_codec = true;
            id3 = audio_current_track();
        }

        /* Save the current playing spot, or NULL if the playlist has ended */
        playlist_update_resume_info(id3);

        /* TODO: Create auto bookmark too? */

        prev_track_elapsed = othertrack_id3->elapsed;

        remove_event(BUFFER_EVENT_BUFFER_LOW, buffering_low_buffer_callback);
    }

    audio_stop_codec_flush();
    paused = false;
    playing = false;
    track_load_started = false;

    filling = STATE_IDLE;

    /* Mark all entries null. */
    audio_clear_track_entries();

    /* Close all tracks */
    audio_release_tracks();
}

static void audio_play_start(size_t offset)
{
    int i;

#if INPUT_SRC_CAPS != 0
    audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif

    /* Wait for any previously playing audio to flush - TODO: Not necessary? */
    paused = false;
    audio_stop_codec_flush();

    playing = true;
    track_load_started = false;

    ci.new_track = 0;
    ci.seek_time = 0;
    wps_offset = 0;

    sound_set_volume(global_settings.volume);
    track_widx = track_ridx = 0;

    /* Clear all track entries. */
    for (i = 0; i < MAX_TRACK; i++) {
        clear_track_info(&tracks[i]);
    }

    last_peek_offset = -1;

    /* Officially playing */
    queue_reply(&audio_queue, 1);

    audio_fill_file_buffer(true, offset);

    add_event(BUFFER_EVENT_BUFFER_LOW, false, buffering_low_buffer_callback);

    LOGFQUEUE("audio > audio Q_AUDIO_TRACK_CHANGED");
    queue_post(&audio_queue, Q_AUDIO_TRACK_CHANGED, 0);
}


/* Invalidates all but currently playing track. */
static void audio_invalidate_tracks(void)
{
    if (audio_have_tracks())
    {
        last_peek_offset = 0;
        track_widx = track_ridx;

        /* Mark all other entries null (also buffered wrong metadata). */
        audio_clear_track_entries();

        track_widx = (track_widx + 1) & MAX_TRACK_MASK;

        audio_fill_file_buffer(false, 0);
        send_event(PLAYBACK_EVENT_TRACK_CHANGE, thistrack_id3);
    }
}

static void audio_new_playlist(void)
{
    /* Prepare to start a new fill from the beginning of the playlist */
    last_peek_offset = -1;
    if (audio_have_tracks())
    {
        if (paused)
            skipped_during_pause = true;
        track_widx = track_ridx;
        audio_clear_track_entries();

        track_widx = (track_widx + 1) & MAX_TRACK_MASK;

        /* Mark the current track as invalid to prevent skipping back to it */
        CUR_TI->taginfo_ready = false;
    }

    /* Signal the codec to initiate a track change forward */
    new_playlist = true;
    ci.new_track = 1;

    /* Officially playing */
    queue_reply(&audio_queue, 1);

    audio_fill_file_buffer(false, 0);
}

/* Called on manual track skip */
static void audio_initiate_track_change(long direction)
{
    logf("audio_initiate_track_change(%ld)", direction);

    ci.new_track += direction;
    wps_offset -= direction;
    if (paused)
        skipped_during_pause = true;
}

/* Called on manual dir skip */
static void audio_initiate_dir_change(long direction)
{
    dir_skip = true;
    ci.new_track = direction;
    if (paused)
        skipped_during_pause = true;
}

/* Called when PCM track change is complete */
static void audio_finalise_track_change(void)
{
    logf("audio_finalise_track_change");

    if (automatic_skip)
    {
        wps_offset = 0;
        automatic_skip = false;

        /* Invalidate prevtrack_id3 */
        memset(othertrack_id3, 0, sizeof(struct mp3entry));

        if (prev_ti && prev_ti->audio_hid < 0)
        {
            /* No audio left so we clear all the track info. */
            clear_track_info(prev_ti);
        }
    }
    send_event(PLAYBACK_EVENT_TRACK_CHANGE, thistrack_id3);
    playlist_update_resume_info(audio_current_track());
}

/*
 * Layout audio buffer as follows - iram buffer depends on target:
 * [|SWAP:iram][|TALK]|FILE|GUARD|PCM|[SWAP:dram[|iram]|]
 */
static void audio_reset_buffer(void)
{
    /* see audio_get_recording_buffer if this is modified */
    logf("audio_reset_buffer");

    /* If the setup of anything allocated before the file buffer is
       changed, do check the adjustments after the buffer_alloc call
       as it will likely be affected and need sliding over */

    /* Initially set up file buffer as all space available */
    malloc_buf = audiobuf + talk_get_bufsize();
    /* Align the malloc buf to line size. Especially important to cf
       targets that do line reads/writes. */
    malloc_buf = (unsigned char *)(((uintptr_t)malloc_buf + 15) & ~15);
    filebuf    = malloc_buf; /* filebuf line align implied */
    filebuflen = audiobufend - filebuf;

    filebuflen &= ~15;

    /* Subtract whatever the pcm buffer says it used plus the guard buffer */
    const size_t pcmbuf_size = pcmbuf_init(filebuf + filebuflen) +GUARD_BUFSIZE;

#ifdef DEBUG
    if(pcmbuf_size > filebuflen)
        panicf("Not enough memory for pcmbuf_init() : %d > %d",
                (int)pcmbuf_size, (int)filebuflen);
#endif

    filebuflen -= pcmbuf_size;

    /* Make sure filebuflen is a longword multiple after adjustment - filebuf
       will already be line aligned */
    filebuflen &= ~3;

    buffering_reset(filebuf, filebuflen);

    /* Clear any references to the file buffer */
    buffer_state = AUDIOBUF_STATE_INITIALIZED;

#if defined(ROCKBOX_HAS_LOGF) && defined(LOGF_ENABLE)
    /* Make sure everything adds up - yes, some info is a bit redundant but
       aids viewing and the sumation of certain variables should add up to
       the location of others. */
    {
        size_t pcmbufsize;
        const unsigned char *pcmbuf = pcmbuf_get_meminfo(&pcmbufsize);
        logf("mabuf:  %08X", (unsigned)malloc_buf);
        logf("fbuf:   %08X", (unsigned)filebuf);
        logf("fbufe:  %08X", (unsigned)(filebuf + filebuflen));
        logf("gbuf:   %08X", (unsigned)(filebuf + filebuflen));
        logf("gbufe:  %08X", (unsigned)(filebuf + filebuflen + GUARD_BUFSIZE));
        logf("pcmb:   %08X", (unsigned)pcmbuf);
        logf("pcmbe:  %08X", (unsigned)(pcmbuf + pcmbufsize));
    }
#endif
}

static void audio_thread(void)
{
    struct queue_event ev;

    pcm_postinit();

    audio_thread_ready = true;

    while (1)
    {
        if (filling != STATE_FILLING && filling != STATE_IDLE) {
            /* End of buffering, let's calculate the watermark and unboost */
            set_filebuf_watermark();
            cancel_cpu_boost();
        }

        if (!pcmbuf_queue_scan(&ev))
            queue_wait_w_tmo(&audio_queue, &ev, HZ/2);

        switch (ev.id) {

            case Q_AUDIO_FILL_BUFFER:
                LOGFQUEUE("audio < Q_AUDIO_FILL_BUFFER %d", (int)ev.data);
                audio_fill_file_buffer((bool)ev.data, 0);
                break;

            case Q_AUDIO_FINISH_LOAD:
                LOGFQUEUE("audio < Q_AUDIO_FINISH_LOAD");
                audio_finish_load_track();
                buf_set_base_handle(CUR_TI->audio_hid);
                break;

            case Q_AUDIO_PLAY:
                LOGFQUEUE("audio < Q_AUDIO_PLAY");
                if (playing && ev.data <= 0)
                    audio_new_playlist();
                else
                {
                    audio_stop_playback();
                    audio_play_start((size_t)ev.data);
                }
                break;

            case Q_AUDIO_STOP:
                LOGFQUEUE("audio < Q_AUDIO_STOP");
                if (playing)
                    audio_stop_playback();
                if (ev.data != 0)
                    queue_clear(&audio_queue);
                break;

            case Q_AUDIO_PAUSE:
                LOGFQUEUE("audio < Q_AUDIO_PAUSE");
                if (!(bool) ev.data && skipped_during_pause
#ifdef HAVE_CROSSFADE
                    && !pcmbuf_is_crossfade_active()
#endif
                    )
                    pcmbuf_play_stop(); /* Flush old track on resume after skip */
                skipped_during_pause = false;
                if (!playing)
                    break;
                pcmbuf_pause((bool)ev.data);
                paused = (bool)ev.data;
                break;

            case Q_AUDIO_SKIP:
                LOGFQUEUE("audio < Q_AUDIO_SKIP");
                audio_initiate_track_change((long)ev.data);
                break;

            case Q_AUDIO_PRE_FF_REWIND:
                LOGFQUEUE("audio < Q_AUDIO_PRE_FF_REWIND");
                if (!playing)
                    break;
                pcmbuf_pause(true);
                break;

            case Q_AUDIO_FF_REWIND:
                LOGFQUEUE("audio < Q_AUDIO_FF_REWIND");
                if (!playing)
                    break;
                if (automatic_skip)
                {
                    /* An automatic track skip is in progress. Finalize it,
                       then go back to the previous track */
                    audio_finalise_track_change();
                    ci.new_track = -1;
                }
                ci.seek_time = (long)ev.data+1;
                break;

            case Q_AUDIO_CHECK_NEW_TRACK:
                LOGFQUEUE("audio < Q_AUDIO_CHECK_NEW_TRACK");
                queue_reply(&audio_queue, audio_check_new_track());
                break;

            case Q_AUDIO_DIR_SKIP:
                LOGFQUEUE("audio < Q_AUDIO_DIR_SKIP");
                audio_initiate_dir_change(ev.data);
                break;

            case Q_AUDIO_FLUSH:
                LOGFQUEUE("audio < Q_AUDIO_FLUSH");
                audio_invalidate_tracks();
                break;

            case Q_AUDIO_TRACK_CHANGED:
                /* PCM track change done */
                LOGFQUEUE("audio < Q_AUDIO_TRACK_CHANGED");
                audio_finalise_track_change();
                break;
#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                LOGFQUEUE("audio < SYS_USB_CONNECTED");
                if (playing)
                    audio_stop_playback();
#ifdef PLAYBACK_VOICE
                voice_stop();
#endif
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                usb_wait_for_disconnect(&audio_queue);

                /* Mark all entries null. */
                audio_clear_track_entries();

                /* release tracks to make sure all handles are closed */
                audio_release_tracks();
                break;
#endif

            case SYS_TIMEOUT:
                LOGFQUEUE_SYS_TIMEOUT("audio < SYS_TIMEOUT");
                break;

            default:
                /* LOGFQUEUE("audio < default : %08lX", ev.id); */
                break;
        } /* end switch */
    } /* end while */
}

/* Initialize the audio system - called from init() in main.c.
 * Last function because of all the references to internal symbols
 */
void audio_init(void)
{
    unsigned int audio_thread_id;

    /* Can never do this twice */
    if (audio_is_initialized)
    {
        logf("audio: already initialized");
        return;
    }

    logf("audio: initializing");

    /* Initialize queues before giving control elsewhere in case it likes
       to send messages. Thread creation will be delayed however so nothing
       starts running until ready if something yields such as talk_init. */
    queue_init(&audio_queue, true);
    queue_init(&codec_queue, false);
    queue_init(&pcmbuf_queue, false);

    pcm_init();

    codec_init_codec_api();
    
    thistrack_id3 = &mp3entry_buf[0];
    othertrack_id3 = &mp3entry_buf[1];
    
    /* cuesheet support */
    if (global_settings.cuesheet)
        curr_cue = (struct cuesheet*)buffer_alloc(sizeof(struct cuesheet));
    
    /* initialize the buffer */
    filebuf = audiobuf;

    /* audio_reset_buffer must to know the size of voice buffer so init
       talk first */
    talk_init();

    make_codec_thread();

    audio_thread_id = create_thread(audio_thread, audio_stack,
                  sizeof(audio_stack), CREATE_THREAD_FROZEN,
                  audio_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE)
                  IF_COP(, CPU));

    queue_enable_queue_send(&audio_queue, &audio_queue_sender_list,
                            audio_thread_id);

#ifdef PLAYBACK_VOICE
    voice_thread_init();
#endif

#ifdef HAVE_CROSSFADE
    /* Set crossfade setting for next buffer init which should be about... */
    pcmbuf_request_crossfade_enable(global_settings.crossfade);
#endif

    /* initialize the buffering system */

    buffering_init();
    /* ...now! Set up the buffers */
    audio_reset_buffer();

    int i;
    for(i = 0; i < MAX_TRACK; i++)
    {
        tracks[i].audio_hid = -1;
        tracks[i].id3_hid = -1;
        tracks[i].codec_hid = -1;
        tracks[i].cuesheet_hid = -1;
    }
#ifdef HAVE_ALBUMART
    FOREACH_ALBUMART(i)
    {
        int j;
        for (j = 0; j < MAX_TRACK; j++)
        {
            tracks[j].aa_hid[i] = -1;
        }
    }
#endif

    add_event(BUFFER_EVENT_REBUFFER, false, buffering_handle_rebuffer_callback);
    add_event(BUFFER_EVENT_FINISHED, false, buffering_handle_finished_callback);

    /* Probably safe to say */
    audio_is_initialized = true;

    sound_settings_apply();
#ifdef HAVE_DISK_STORAGE
    audio_set_buffer_margin(global_settings.buffer_margin);
#endif

    /* it's safe to let the threads run now */
#ifdef PLAYBACK_VOICE
    voice_thread_resume();
#endif
    thread_thaw(codec_thread_id);
    thread_thaw(audio_thread_id);

} /* audio_init */

bool audio_is_thread_ready(void)
{
    return audio_thread_ready;
}

size_t audio_get_filebuflen(void)
{
    return filebuflen;
}

int get_audio_hid()
{
    return CUR_TI->audio_hid;
}

int *get_codec_hid()
{
    return &tracks[track_ridx].codec_hid;
}
