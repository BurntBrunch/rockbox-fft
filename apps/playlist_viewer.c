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
 * Copyright (C) 2003 Hardeep Sidhu
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
/*
 * Kevin Ferrare 2005/10/16
 * multi-screen support, rewrote a lot of code
 */
#include <string.h>
#include <sprintf.h>
#include "playlist.h"
#include "audio.h"
#include "screens.h"
#include "settings.h"
#include "icons.h"
#include "menu.h"
#include "plugin.h"
#include "keyboard.h"
#include "filetypes.h"
#include "onplay.h"
#include "talk.h"
#include "misc.h"
#include "action.h"
#include "debug.h"
#include "backlight.h"

#include "lang.h"

#include "playlist_viewer.h"
#include "playlist_catalog.h"
#include "icon.h"
#include "list.h"
#include "splash.h"
#include "playlist_menu.h"

/* Maximum number of tracks we can have loaded at one time */
#define MAX_PLAYLIST_ENTRIES 200

/* The number of items between the selected one and the end/start of
 * the buffer under which the buffer must reload */
#define MIN_BUFFER_MARGIN (screens[0].getnblines()+1)

/* Information about a specific track */
struct playlist_entry {
    char *name;                 /* Formatted track name                     */
    int index;                  /* Playlist index                           */
    int display_index;          /* Display index                            */
    bool queued;                /* Is track queued?                         */
    bool skipped;               /* Is track marked as bad?                  */
};

enum direction
{
    FORWARD,
    BACKWARD
};

struct playlist_buffer
{
    char *name_buffer;        /* Buffer used to store track names */
    int buffer_size;          /* Size of name buffer */

    int first_index;          /* Real index of first track loaded inside
                                 the buffer */

    enum direction direction; /* Direction of the buffer (if the buffer
                                 was loaded BACKWARD, the last track in
                                 the buffer has a real index < to the
                                 real index of the the first track)*/

    struct playlist_entry tracks[MAX_PLAYLIST_ENTRIES];
    int num_loaded;           /* Number of track entries loaded in buffer */
};

/* Global playlist viewer settings */
struct playlist_viewer {
    struct playlist_info* playlist; /* playlist being viewed                 */
    int num_tracks;             /* Number of tracks in playlist              */
    int current_playing_track;  /* Index of current playing track            */
    int selected_track;         /* The selected track, relative (first is 0) */
    int moving_track;           /* The track to move, relative (first is 0)
                                   or -1 if nothing is currently being moved */
    int moving_playlist_index;  /* Playlist-relative index (as opposed to 
                                   viewer-relative index) of moving track    */
    struct playlist_buffer buffer;
};

static struct playlist_viewer  viewer;

/* Used when viewing playlists on disk */
static struct playlist_info temp_playlist;

static void playlist_buffer_init(struct playlist_buffer *pb, char *names_buffer,
                                 int names_buffer_size);
static void playlist_buffer_load_entries(struct playlist_buffer * pb, int index,
                                         enum direction direction);
static int playlist_entry_load(struct playlist_entry *entry, int index,
                               char* name_buffer, int remaining_size);

static struct playlist_entry * playlist_buffer_get_track(struct playlist_buffer *pb,
                                                         int index);

static bool playlist_viewer_init(struct playlist_viewer * viewer,
                                 const char* filename, bool reload);

static void format_name(char* dest, const char* src);
static void format_line(const struct playlist_entry* track, char* str,
                        int len);

static bool update_playlist(bool force);
static int  onplay_menu(int index);
static bool viewer_menu(void);
static int save_playlist_func(void);

static void playlist_buffer_init(struct playlist_buffer *pb, char *names_buffer,
                                 int names_buffer_size)
{
    pb->name_buffer=names_buffer;
    pb->buffer_size=names_buffer_size;
    pb->first_index=0;
    pb->num_loaded=0;
}

/*
 * Loads the entries following 'index' in the playlist buffer
 */
static void playlist_buffer_load_entries(struct playlist_buffer *pb, int index,
                                         enum direction direction)
{
    int num_entries = viewer.num_tracks;
    char* p = pb->name_buffer;
    int remaining = pb->buffer_size;
    int i;

    pb->first_index = index;
    if (num_entries > MAX_PLAYLIST_ENTRIES)
        num_entries = MAX_PLAYLIST_ENTRIES;

    for(i=0; i<num_entries; i++)
    {
        int len = playlist_entry_load(&(pb->tracks[i]), index, p, remaining);
        if (len < 0)
        {
            /* Out of name buffer space */
            num_entries = i;
            break;
        }

        p += len;
        remaining -= len;

        if(direction==FORWARD)
            index++;
        else
            index--;
        index+=viewer.num_tracks;
        index%=viewer.num_tracks;
    }
    pb->direction=direction;
    pb->num_loaded = i;
}

static void playlist_buffer_load_entries_screen(struct playlist_buffer * pb,
                                                enum direction direction)
{
    if(direction==FORWARD)
    {
        int min_start=viewer.selected_track-2*screens[0].getnblines();
        while(min_start<0)
            min_start+=viewer.num_tracks;
        min_start %= viewer.num_tracks;
        playlist_buffer_load_entries(pb, min_start, FORWARD);
    }
     else
    {
        int max_start=viewer.selected_track+2*screens[0].getnblines();
        max_start%=viewer.num_tracks;
        playlist_buffer_load_entries(pb, max_start, BACKWARD);
    }
}

static int playlist_entry_load(struct playlist_entry *entry, int index,
                               char* name_buffer, int remaining_size)
{
    struct playlist_track_info info;
    int len;

    /* Playlist viewer orders songs based on display index.  We need to
       convert to real playlist index to access track */
    index = (index + playlist_get_first_index(viewer.playlist)) %
        viewer.num_tracks;
    if (playlist_get_track_info(viewer.playlist, index, &info) < 0)
        return -1;

    len = strlen(info.filename) + 1;

    if (len <= remaining_size)
    {
        strcpy(name_buffer, info.filename);

        entry->name = name_buffer;
        entry->index = info.index;
        entry->display_index = info.display_index;
        entry->queued = info.attr & PLAYLIST_ATTR_QUEUED;
        entry->skipped = info.attr & PLAYLIST_ATTR_SKIPPED;
        return len;
    }
    return -1;
}

static int playlist_buffer_get_index(struct playlist_buffer *pb, int index)
{
    int buffer_index;
    if(pb->direction==FORWARD)
    {
        if(index>=pb->first_index)
            buffer_index=index-pb->first_index;
        else /* rotation : track0 in buffer + requested track */
            buffer_index=(viewer.num_tracks-pb->first_index)+(index);
    }
    else
    {
        if(index<=pb->first_index)
            buffer_index=pb->first_index-index;
        else /* rotation : track0 in buffer + dist from the last track
                to the requested track (num_tracks-requested track) */
            buffer_index=(pb->first_index)+(viewer.num_tracks-index);
    }
    return(buffer_index);
}

#define distance(a, b) \
    a>b? (a) - (b) : (b) - (a)
static bool playlist_buffer_needs_reload(struct playlist_buffer* pb,
                                         int track_index)
{
    if(pb->num_loaded==viewer.num_tracks)
        return(false);
    int selected_index=playlist_buffer_get_index(pb, track_index);
    int first_buffer_index=playlist_buffer_get_index(pb, pb->first_index);
    int distance_beginning=distance(selected_index, first_buffer_index);
    if(distance_beginning<MIN_BUFFER_MARGIN)
        return(true);

    if(pb->num_loaded - distance_beginning < MIN_BUFFER_MARGIN)
       return(true);
    return(false);
}

static struct playlist_entry * playlist_buffer_get_track(struct playlist_buffer *pb,
                                                         int index)
{
    int buffer_index=playlist_buffer_get_index(pb, index);
    return(&(pb->tracks[buffer_index]));
}

/* Initialize the playlist viewer. */
static bool playlist_viewer_init(struct playlist_viewer * viewer,
                                 const char* filename, bool reload)
{
    char* buffer;
    size_t buffer_size;
    bool is_playing = audio_status() & (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE);
    bool have_list = filename || is_playing;
    if (!have_list && (global_status.resume_index != -1))
    {
        /* Try to restore the list from control file */
        have_list = (playlist_resume() != -1);
    }
    if (!have_list && (playlist_amount() > 0))
    {
         /*If dynamic playlist still exists, view it anyway even 
        if playback has reached the end of the playlist */
        have_list = true;
    }
    if (!have_list)
    {
        /* Nothing to view, exit */
        splash(HZ, str(LANG_CATALOG_NO_PLAYLISTS));
        return false;
    }

    buffer = plugin_get_buffer(&buffer_size);
    if (!buffer)
        return false;

    if (!filename)
        viewer->playlist = NULL;
    else
    {
        /* Viewing playlist on disk */
        const char *dir, *file;
        char *temp_ptr;
        char *index_buffer = NULL;
        ssize_t index_buffer_size = 0;

        viewer->playlist = &temp_playlist;

        /* Separate directory from filename */
        temp_ptr = strrchr(filename+1,'/');
        if (temp_ptr)
        {
            *temp_ptr = 0;
            dir = filename;
            file = temp_ptr + 1;
        }
        else
        {
            dir = "/";
            file = filename+1;
        }

        if (is_playing)
        {
            /* Something is playing, use half the plugin buffer for playlist
               indices */
            index_buffer_size = buffer_size / 2;
            index_buffer = buffer;
        }

        playlist_create_ex(viewer->playlist, dir, file, index_buffer,
            index_buffer_size, buffer+index_buffer_size,
            buffer_size-index_buffer_size);

        if (temp_ptr)
            *temp_ptr = '/';

        buffer += index_buffer_size;
        buffer_size -= index_buffer_size;
    }
    playlist_buffer_init(&viewer->buffer, buffer, buffer_size );

    viewer->moving_track = -1;
    viewer->moving_playlist_index = -1;

    if (!reload)
    {
        if (viewer->playlist)
            viewer->selected_track = 0;
        else
            viewer->selected_track = playlist_get_display_index() - 1;
    }

    if (!update_playlist(true))
        return false;
    return true;
}

/* Format trackname for display purposes */
static void format_name(char* dest, const char* src)
{
    switch (global_settings.playlist_viewer_track_display)
    {
        case 0:
        default:
        {
            /* Only display the filename */
            char* p = strrchr(src, '/');

            strcpy(dest, p+1);

            /* Remove the extension */
            strrsplt(dest, '.');

            break;
        }
        case 1:
            /* Full path */
            strcpy(dest, src);
            break;
    }
}

/* Format display line */
static void format_line(const struct playlist_entry* track, char* str,
                        int len)
{
    char name[MAX_PATH];
    char *skipped = "";

    format_name(name, track->name);

    if (track->skipped)
        skipped = "(ERR) ";

    if (global_settings.playlist_viewer_indices)
        /* Display playlist index */
        snprintf(str, len, "%d. %s%s", track->display_index, skipped, name);
    else
        snprintf(str, len, "%s%s", skipped, name);

}

/* Update playlist in case something has changed or forced */
static bool update_playlist(bool force)
{
    if (!viewer.playlist)
        playlist_get_resume_info(&viewer.current_playing_track);
    else
        viewer.current_playing_track = -1;
    int nb_tracks=playlist_amount_ex(viewer.playlist);
    force=force || nb_tracks != viewer.num_tracks;
    if (force)
    {
        /* Reload tracks */
        viewer.num_tracks = nb_tracks;
        if (viewer.num_tracks <= 0)
        {
            global_status.resume_index = -1;
            global_status.resume_offset = -1;
            return false;
        }
        playlist_buffer_load_entries_screen(&viewer.buffer, FORWARD);
        if (viewer.buffer.num_loaded <= 0)
        {
            global_status.resume_index = -1;
            global_status.resume_offset = -1;
            return false;
        }
    }
    return true;
}

/* Menu of playlist commands.  Invoked via ON+PLAY on main viewer screen.
   Returns -1 if USB attached, 0 if no playlist change, and 1 if playlist
   changed. */
static int onplay_menu(int index)
{
    int result, ret = 0;
    struct playlist_entry * current_track=
        playlist_buffer_get_track(&viewer.buffer, index);
    MENUITEM_STRINGLIST(menu_items, ID2P(LANG_PLAYLIST), NULL, 
                        ID2P(LANG_REMOVE), ID2P(LANG_MOVE),
                        ID2P(LANG_CATALOG_ADD_TO), ID2P(LANG_CATALOG_ADD_TO_NEW));
    bool current = (current_track->index == viewer.current_playing_track);

    result = do_menu(&menu_items, NULL, NULL, false);
    if (result == MENU_ATTACHED_USB)
    {
        ret = -1;
    }
    else if (result >= 0)
    {
        /* Abort current move */
        viewer.moving_track = -1;
        viewer.moving_playlist_index = -1;

        switch (result)
        {
            case 0:
                /* delete track */
                playlist_delete(viewer.playlist, current_track->index);
                if (current)
                {
                    if (playlist_amount_ex(viewer.playlist) <= 0)
                        audio_stop();
                    else
                    {
                       /* Start playing new track except if it's the lasttrack
                          track in the playlist and repeat mode is disabled */
                        current_track =
                            playlist_buffer_get_track(&viewer.buffer, index);
                        if (current_track->display_index!=viewer.num_tracks ||
                            global_settings.repeat_mode == REPEAT_ALL)
                        {
#if CONFIG_CODEC != SWCODEC
                            talk_buffer_steal(); /* will use the mp3 buffer */
#endif
                            audio_play(0);
                            viewer.current_playing_track = -1;
                        }
                    }
                }
                ret = 1;
                break;
            case 1:
                /* move track */
                viewer.moving_track = index;
                viewer.moving_playlist_index = current_track->index;
                ret = 0;
                break;
            case 2: /* add to catalog */
            case 3: /* add to a new one */
                catalog_add_to_a_playlist(current_track->name,
                                          FILE_ATTR_AUDIO,
                                          result==3, NULL);
                ret = 0;
                break;
        }
    }
    return ret;
}

/* Menu of viewer options.  Invoked via F1(r) or Menu(p). */
MENUITEM_SETTING(show_icons, &global_settings.playlist_viewer_icons, NULL);
MENUITEM_SETTING(show_indices, &global_settings.playlist_viewer_indices, NULL);
MENUITEM_SETTING(track_display, 
                 &global_settings.playlist_viewer_track_display, NULL);
MENUITEM_FUNCTION(save_playlist_item, 0, ID2P(LANG_SAVE_DYNAMIC_PLAYLIST),
                  save_playlist_func, 0, NULL, Icon_NOICON);
MAKE_MENU(viewer_settings_menu, ID2P(LANG_PLAYLISTVIEWER_SETTINGS), 
          NULL, Icon_Playlist,
          &show_icons, &show_indices, &track_display, &save_playlist_item);
static bool viewer_menu(void)
{
    return do_menu(&viewer_settings_menu, NULL, NULL, false) == MENU_ATTACHED_USB;
}

/* Save playlist to disk */
static int save_playlist_func(void)
{
    save_playlist_screen(viewer.playlist);
    return 0;
}

/* View current playlist */
bool playlist_viewer(void)
{
    return playlist_viewer_ex(NULL);
}

static int get_track_num( struct playlist_viewer * local_viewer,
                          int selected_item )
{
    if( local_viewer->moving_track >= 0 )
    {
        if( local_viewer->selected_track == selected_item )
        {
            return local_viewer->moving_track;
        }
        else if( local_viewer->selected_track > selected_item
             && selected_item >= local_viewer->moving_track )
        {
            return selected_item+1; /* move down */
        }
        else if( local_viewer->selected_track < selected_item
              && selected_item <= local_viewer->moving_track )
        {
            return selected_item-1; /* move up */
        }
    }
    return selected_item;
}

static const char* playlist_callback_name(int selected_item,
                                          void *data,
                                          char *buffer,
                                          size_t buffer_len)
{
    struct playlist_viewer * local_viewer = (struct playlist_viewer *)data;

    int track_num = get_track_num(local_viewer, selected_item);
    struct playlist_entry *track =
       playlist_buffer_get_track(&(local_viewer->buffer), track_num);

    format_line(track, buffer, buffer_len);

    return(buffer);
}


static enum themable_icons playlist_callback_icons(int selected_item,
                                                   void *data)
{
    struct playlist_viewer * local_viewer=(struct playlist_viewer *)data;

    int track_num = get_track_num(local_viewer, selected_item);
    struct playlist_entry *track=
        playlist_buffer_get_track(&(local_viewer->buffer), track_num);

    if (track->index == local_viewer->current_playing_track)
    {
        /* Current playing track */
        return Icon_Audio;
    }
    else if (track->index == local_viewer->moving_playlist_index)
    {
        /* Track we are moving */
        return Icon_Moving;
    }
    else if (track->queued)
    {
        /* Queued track */
        return Icon_Queued;
    }
    else
        return Icon_NOICON;
}

/* Main viewer function.  Filename identifies playlist to be viewed.  If NULL,
   view current playlist. */
bool playlist_viewer_ex(const char* filename)
{
    bool ret = false;       /* return value */
    bool exit=false;        /* exit viewer */
    int button;
    struct gui_synclist playlist_lists;
    if (!playlist_viewer_init(&viewer, filename, false))
        goto exit;

    gui_synclist_init(&playlist_lists, playlist_callback_name,
                      &viewer, false, 1, NULL);
    gui_synclist_set_icon_callback(&playlist_lists,
                  global_settings.playlist_viewer_icons?
                  &playlist_callback_icons:NULL);
    gui_synclist_set_nb_items(&playlist_lists, viewer.num_tracks);
    gui_synclist_set_title(&playlist_lists, str(LANG_PLAYLIST), Icon_Playlist);
    gui_synclist_select_item(&playlist_lists, viewer.selected_track);
    gui_synclist_draw(&playlist_lists);
    while (!exit)
    {
        int track;

        if (global_status.resume_index != -1 && !viewer.playlist)
            playlist_get_resume_info(&track);
        else
            track = -1;

        if (track != viewer.current_playing_track ||
            playlist_amount_ex(viewer.playlist) != viewer.num_tracks)
        {
            /* Playlist has changed (new track started?) */
            if (!update_playlist(false))
                goto exit;
            /*Needed because update_playlist gives wrong value when
                                                            playing is stopped*/
            viewer.current_playing_track = track;
            gui_synclist_set_nb_items(&playlist_lists, viewer.num_tracks);

            gui_synclist_draw(&playlist_lists);
        }

        /* Timeout so we can determine if play status has changed */
        button = get_action(CONTEXT_TREE,HZ/2);
        if( (gui_synclist_do_button(&playlist_lists, &button,LIST_WRAP_UNLESS_HELD)) )
        {
            viewer.selected_track=gui_synclist_get_sel_pos(&playlist_lists);
            if(playlist_buffer_needs_reload(&viewer.buffer,
                viewer.selected_track))
                playlist_buffer_load_entries_screen(&viewer.buffer,
                    button==ACTION_STD_NEXT?
                        FORWARD
                        :
                        BACKWARD
                    );
            gui_synclist_draw(&playlist_lists);
        }
        switch (button)
        {
            case ACTION_TREE_WPS:
            case ACTION_STD_CANCEL:
            {
                if (viewer.moving_track >= 0)
                {
                    viewer.selected_track = viewer.moving_track;
                    gui_synclist_select_item(&playlist_lists, viewer.moving_track);
                    viewer.moving_track = -1;
                    viewer.moving_playlist_index = -1;
                    gui_synclist_draw(&playlist_lists);
                }
                else
                    exit = true;
                break;
            }
            case ACTION_STD_OK:
            {
                struct playlist_entry * current_track =
                    playlist_buffer_get_track(&viewer.buffer,
                    viewer.selected_track);
                if (viewer.moving_track >= 0)
                {
                    /* Move track */
                    int ret_val;

                    ret_val = playlist_move(viewer.playlist,
                                            viewer.moving_playlist_index,
                                            current_track->index);
                    if (ret_val < 0)
                         splashf(HZ, (unsigned char *)"%s %s", str(LANG_MOVE),
                                                               str(LANG_FAILED));
                    update_playlist(true);
                    viewer.moving_track = -1;
                    viewer.moving_playlist_index = -1;
                }
                else if (!viewer.playlist)
                {
                    /* play new track */
                    playlist_start(current_track->index, 0);
                    update_playlist(false);
                }
                else
                {
                    /* New playlist */
                    if (playlist_set_current(viewer.playlist) < 0)
                        goto exit;

                    playlist_start(current_track->index, 0);

                    /* Our playlist is now the current list */
                    if (!playlist_viewer_init(&viewer, NULL, true))
                        goto exit;
                }
                gui_synclist_draw(&playlist_lists);

                break;
            }
            case ACTION_STD_CONTEXT:
            {
                /* ON+PLAY menu */
                int ret_val;

                ret_val = onplay_menu(viewer.selected_track);

                if (ret_val < 0)
                {
                    ret = true;
                    goto exit;
                }
                else if (ret_val > 0)
                {
                    /* Playlist changed */
                    gui_synclist_del_item(&playlist_lists);
                    update_playlist(true);
                    if (viewer.num_tracks <= 0)
                        exit = true;
                    if(viewer.selected_track >= viewer.num_tracks)
                        viewer.selected_track = viewer.num_tracks-1;
                }
                gui_synclist_draw(&playlist_lists);
                break;
            }
            case ACTION_STD_MENU:
                if (viewer_menu())
                {
                    ret = true;
                    goto exit;
                }
                gui_synclist_set_icon_callback(
                    &playlist_lists,
                    global_settings.playlist_viewer_icons?
                        &playlist_callback_icons:NULL
                    );
                gui_synclist_draw(&playlist_lists);
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    ret = true;
                    goto exit;
                }
                break;
        }
    }

exit:
    if (viewer.playlist)
        playlist_close(viewer.playlist);
    return ret;
}

static const char* playlist_search_callback_name(int selected_item, void * data,
                                                 char *buffer, size_t buffer_len)
{
    (void)buffer_len; /* this should probably be used */
    int *found_indicies = (int*)data;
    static struct playlist_track_info track;
    playlist_get_track_info(viewer.playlist, found_indicies[selected_item], &track);
    format_name(buffer, track.filename);
    return buffer;
}

bool search_playlist(void)
{
    char search_str[32] = "";
    bool ret = false, exit = false;
    int i, playlist_count;
    int found_indicies[MAX_PLAYLIST_ENTRIES];
    int found_indicies_count = 0, last_found_count = -1;
    int button;
    struct gui_synclist playlist_lists;
    struct playlist_track_info track;

    if (!playlist_viewer_init(&viewer, 0, false))
        return ret;
    if (kbd_input(search_str, sizeof(search_str)) < 0)
        return ret;
    lcd_clear_display();
    playlist_count = playlist_amount_ex(viewer.playlist);

    cpu_boost(true);

    for (i=0; (i<playlist_count)&&(found_indicies_count<MAX_PLAYLIST_ENTRIES); i++)
    {
        if (found_indicies_count != last_found_count)
        {
            splashf(0, str(LANG_PLAYLIST_SEARCH_MSG), found_indicies_count,
                       str(LANG_OFF_ABORT));
            last_found_count = found_indicies_count;
        }

        if (action_userabort(TIMEOUT_NOBLOCK))
            break;

        playlist_get_track_info(viewer.playlist, i, &track);

        if (strcasestr(track.filename,search_str))
            found_indicies[found_indicies_count++] = track.index;

        yield();
    }

    cpu_boost(false);

    if (!found_indicies_count)
    {
        return ret;
    }
    backlight_on();

    gui_synclist_init(&playlist_lists, playlist_search_callback_name,
                      found_indicies, false, 1, NULL);
    gui_synclist_set_title(&playlist_lists, str(LANG_SEARCH_RESULTS), NOICON);
    gui_synclist_set_icon_callback(&playlist_lists, NULL);
    gui_synclist_set_nb_items(&playlist_lists, found_indicies_count);
    gui_synclist_select_item(&playlist_lists, 0);
    gui_synclist_draw(&playlist_lists);
    while (!exit)
    {
        button = get_action(CONTEXT_LIST, HZ/4);
        if (gui_synclist_do_button(&playlist_lists, &button, LIST_WRAP_UNLESS_HELD))
            continue;
        switch (button)
        {
            case ACTION_STD_CANCEL:
                exit = true;
                break;

            case ACTION_STD_OK:
                playlist_start(
                    found_indicies[gui_synclist_get_sel_pos(&playlist_lists)]
                    ,0);
                exit = 1;
            break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                {
                    ret = true;
                    exit = true;
                }
                break;
        }
    }
    return ret;
}

