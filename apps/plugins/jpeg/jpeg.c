/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* JPEG image viewer
* (This is a real mess if it has to be coded in one single C file)
*
* File scrolling addition (C) 2005 Alexander Spyridakis
* Copyright (C) 2004 Jörg Hohensohn aka [IDC]Dragon
* Heavily borrowed from the IJG implementation (C) Thomas G. Lane
* Small & fast downscaling IDCT (C) 2002 by Guido Vollbeding  JPEGclub.org
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
#include <lib/playback_control.h>
#include <lib/helper.h>
#include <lib/configfile.h>

#include <lib/grey.h>
#include <lib/xlcd.h>

#include "jpeg.h"
#include "jpeg_decoder.h"

PLUGIN_HEADER

#ifdef HAVE_LCD_COLOR
#include "yuv2rgb.h"
#endif

/* different graphics libraries */
#if LCD_DEPTH < 8
#define USEGSLIB
GREY_INFO_STRUCT
#define MYLCD(fn) grey_ub_ ## fn
#define MYLCD_UPDATE()
#define MYXLCD(fn) grey_ub_ ## fn
#else
#define MYLCD(fn) rb->lcd_ ## fn
#define MYLCD_UPDATE() rb->lcd_update();
#define MYXLCD(fn) xlcd_ ## fn
#endif

/* Min memory allowing us to use the plugin buffer
 * and thus not stopping the music
 * *Very* rough estimation:
 * Max 10 000 dir entries * 4bytes/entry (char **) = 40000 bytes
 * + 20k code size = 60 000
 * + 50k min for jpeg = 120 000
 */
#define MIN_MEM 120000

/* Headings */
#define DIR_PREV  1
#define DIR_NEXT -1
#define DIR_NONE  0

#define PLUGIN_OTHER    10 /* State code for output with return. */
#define PLUGIN_ABORT    11
#define PLUGIN_OUTOFMEM 12

/******************************* Globals ***********************************/

static int slideshow_enabled = false;   /* run slideshow */
static int running_slideshow = false;   /* loading image because of slideshw */
#ifndef SIMULATOR
static int immediate_ata_off = false;   /* power down disk after loading */
#endif

#ifdef HAVE_LCD_COLOR
fb_data rgb_linebuf[LCD_WIDTH];  /* Line buffer for scrolling when
                                    DITHER_DIFFUSION is set                */
#endif


/* Persistent configuration */
#define JPEG_CONFIGFILE             "jpeg.cfg"
#define JPEG_SETTINGS_MINVERSION    1
#define JPEG_SETTINGS_VERSION       2

/* Slideshow times */
#define SS_MIN_TIMEOUT 1
#define SS_MAX_TIMEOUT 20
#define SS_DEFAULT_TIMEOUT 5

struct jpeg_settings
{
#ifdef HAVE_LCD_COLOR
    int colour_mode;
    int dither_mode;
#endif
    int ss_timeout;
};

static struct jpeg_settings jpeg_settings =
{
#ifdef HAVE_LCD_COLOR
    COLOURMODE_COLOUR,
    DITHER_NONE,
#endif
    SS_DEFAULT_TIMEOUT
};
static struct jpeg_settings old_settings;

static struct configdata jpeg_config[] =
{
#ifdef HAVE_LCD_COLOR
    { TYPE_ENUM, 0, COLOUR_NUM_MODES, { .int_p = &jpeg_settings.colour_mode },
        "Colour Mode", (char *[]){ "Colour", "Grayscale" } },
    { TYPE_ENUM, 0, DITHER_NUM_MODES, { .int_p = &jpeg_settings.dither_mode },
        "Dither Mode", (char *[]){ "None", "Ordered", "Diffusion" } },
#endif
    { TYPE_INT, SS_MIN_TIMEOUT, SS_MAX_TIMEOUT,
        { .int_p = &jpeg_settings.ss_timeout }, "Slideshow Time", NULL },
};

#if LCD_DEPTH > 1
static fb_data* old_backdrop;
#endif

/**************** begin Application ********************/


/************************* Types ***************************/

struct t_disp
{
#ifdef HAVE_LCD_COLOR
    unsigned char* bitmap[3]; /* Y, Cr, Cb */
    int csub_x, csub_y;
#else
    unsigned char* bitmap[1]; /* Y only */
#endif
    int width;
    int height;
    int stride;
    int x, y;
};

/************************* Globals ***************************/

/* decompressed image in the possible sizes (1,2,4,8), wasting the other */
static struct t_disp disp[9];

/* my memory pool (from the mp3 buffer) */
static char print[32]; /* use a common snprintf() buffer */
/* the remaining free part of the buffer for compressed+uncompressed images */
static unsigned char* buf;
static ssize_t buf_size;

/* the root of the images, hereafter are decompresed ones */
static unsigned char* buf_root;
static int root_size;

/* up to here currently used by image(s) */
static unsigned char* buf_images;
static ssize_t buf_images_size;

static int ds, ds_min, ds_max; /* downscaling and limits */
static struct jpeg jpg; /* too large for stack */

static struct tree_context *tree;

/* the current full file name */
static char np_file[MAX_PATH];
static int curfile = 0, direction = DIR_NONE, entries = 0;

/* list of the jpeg files */
static char **file_pt;
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
/* are we using the plugin buffer or the audio buffer? */
static bool plug_buf = true;
#endif


/************************* Implementation ***************************/

bool jpg_ext(const char ext[])
{
    if(!ext)
        return false;
    if(!rb->strcasecmp(ext,".jpg") ||
       !rb->strcasecmp(ext,".jpe") ||
       !rb->strcasecmp(ext,".jpeg"))
            return true;
    else
            return false;
}

/*Read directory contents for scrolling. */
void get_pic_list(void)
{
    int i;
    struct entry *dircache;
    char *pname;
    tree = rb->tree_get_context();
    dircache = tree->dircache;

    file_pt = (char **) buf;

    /* Remove path and leave only the name.*/
    pname = rb->strrchr(np_file,'/');
    pname++;

    for (i = 0; i < tree->filesindir; i++)
    {
        if (!(dircache[i].attr & ATTR_DIRECTORY)
            && jpg_ext(rb->strrchr(dircache[i].name,'.')))
        {
            file_pt[entries] = dircache[i].name;
            /* Set Selected File. */
            if (!rb->strcmp(file_pt[entries], pname))
                curfile = entries;
            entries++;
        }
    }

    buf += (entries * sizeof(char**));
    buf_size -= (entries * sizeof(char**));
}

int change_filename(int direct)
{
    bool file_erased = (file_pt[curfile] == NULL);
    direction = direct;

    curfile += (direct == DIR_PREV? entries - 1: 1);
    if (curfile >= entries)
        curfile -= entries;

    if (file_erased)
    {
        /* remove 'erased' file names from list. */
        int count, i;
        for (count = i = 0; i < entries; i++)
        {
            if (curfile == i)
                curfile = count;
            if (file_pt[i] != NULL)
                file_pt[count++] = file_pt[i];
        }
        entries = count;
    }

    if (entries == 0)
    {
        rb->splash(HZ, "No supported files");
        return PLUGIN_ERROR;
    }

    if (rb->strlen(tree->currdir) > 1)
    {
        rb->strcpy(np_file, tree->currdir);
        rb->strcat(np_file, "/");
    }
    else
        rb->strcpy(np_file, tree->currdir);

    rb->strcat(np_file, file_pt[curfile]);

    return PLUGIN_OTHER;
}

/* switch off overlay, for handling SYS_ events */
void cleanup(void *parameter)
{
    (void)parameter;
#ifdef USEGSLIB
    grey_show(false);
#endif
}

#define VSCROLL (LCD_HEIGHT/8)
#define HSCROLL (LCD_WIDTH/10)

#define ZOOM_IN  100 /* return codes for below function */
#define ZOOM_OUT 101

#ifdef HAVE_LCD_COLOR
bool set_option_grayscale(void)
{
    bool gray = jpeg_settings.colour_mode == COLOURMODE_GRAY;
    rb->set_bool("Grayscale", &gray);
    jpeg_settings.colour_mode = gray ? COLOURMODE_GRAY : COLOURMODE_COLOUR;
    return false;
}

bool set_option_dithering(void)
{
    static const struct opt_items dithering[DITHER_NUM_MODES] = {
        [DITHER_NONE]      = { "Off",       -1 },
        [DITHER_ORDERED]   = { "Ordered",   -1 },
        [DITHER_DIFFUSION] = { "Diffusion", -1 },
    };

    rb->set_option("Dithering", &jpeg_settings.dither_mode, INT,
                   dithering, DITHER_NUM_MODES, NULL);
    return false;
}

MENUITEM_FUNCTION(grayscale_item, 0, "Greyscale",
                  set_option_grayscale, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(dithering_item, 0, "Dithering",
                  set_option_dithering, NULL, NULL, Icon_NOICON);
MAKE_MENU(display_menu, "Display Options", NULL, Icon_NOICON,
            &grayscale_item, &dithering_item);

static void display_options(void)
{
    rb->do_menu(&display_menu, NULL, NULL, false);
}
#endif /* HAVE_LCD_COLOR */

int show_menu(void) /* return 1 to quit */
{
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(old_backdrop);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(rb->global_settings->fg_color);
    rb->lcd_set_background(rb->global_settings->bg_color);
#else
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_WHITE);
#endif
#endif
    int result;

    enum menu_id
    {
        MIID_RETURN = 0,
        MIID_TOGGLE_SS_MODE,
        MIID_CHANGE_SS_MODE,
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        MIID_SHOW_PLAYBACK_MENU,
#endif
#ifdef HAVE_LCD_COLOR
        MIID_DISPLAY_OPTIONS,
#endif
        MIID_QUIT,
    };

    MENUITEM_STRINGLIST(menu, "Jpeg Menu", NULL,
                        "Return", "Toggle Slideshow Mode",
                        "Change Slideshow Time",
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
                        "Show Playback Menu",
#endif
#ifdef HAVE_LCD_COLOR
                        "Display Options",
#endif
                        "Quit");

    static const struct opt_items slideshow[2] = {
        { "Disable", -1 },
        { "Enable", -1 },
    };

    result=rb->do_menu(&menu, NULL, NULL, false);

    switch (result)
    {
        case MIID_RETURN:
            break;
        case MIID_TOGGLE_SS_MODE:
            rb->set_option("Toggle Slideshow", &slideshow_enabled, INT,
                           slideshow , 2, NULL);
            break;
        case MIID_CHANGE_SS_MODE:
            rb->set_int("Slideshow Time", "s", UNIT_SEC,
                        &jpeg_settings.ss_timeout, NULL, 1,
                        SS_MIN_TIMEOUT, SS_MAX_TIMEOUT, NULL);
            break;

#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        case MIID_SHOW_PLAYBACK_MENU:
            if (plug_buf)
            {
                playback_control(NULL);
            }
            else
            {
                rb->splash(HZ, "Cannot restart playback");
            }
            break;
#endif
#ifdef HAVE_LCD_COLOR
        case MIID_DISPLAY_OPTIONS:
            display_options();
            break;
#endif
        case MIID_QUIT:
            return 1;
            break;
    }

#if !defined(SIMULATOR) && defined(HAVE_DISK_STORAGE)
    /* change ata spindown time based on slideshow time setting */
    immediate_ata_off = false;
    rb->storage_spindown(rb->global_settings->disk_spindown);

    if (slideshow_enabled)
    {
        if(jpeg_settings.ss_timeout < 10)
        {
            /* slideshow times < 10s keep disk spinning */
            rb->storage_spindown(0);
        }
        else if (!rb->mp3_is_playing())
        {
            /* slideshow times > 10s and not playing: ata_off after load */
            immediate_ata_off = true;
        }
    }
#endif
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif
    rb->lcd_clear_display();
    return 0;
}

void draw_image_rect(struct t_disp* pdisp, int x, int y, int width, int height)
{
#ifdef HAVE_LCD_COLOR
    yuv_bitmap_part(
        pdisp->bitmap, pdisp->csub_x, pdisp->csub_y,
        pdisp->x + x, pdisp->y + y, pdisp->stride,
        x + MAX(0, (LCD_WIDTH - pdisp->width) / 2),
        y + MAX(0, (LCD_HEIGHT - pdisp->height) / 2),
        width, height,
        jpeg_settings.colour_mode, jpeg_settings.dither_mode);
#else
    MYXLCD(gray_bitmap_part)(
        pdisp->bitmap[0], pdisp->x + x, pdisp->y + y, pdisp->stride,
        x + MAX(0, (LCD_WIDTH-pdisp->width)/2),
        y + MAX(0, (LCD_HEIGHT-pdisp->height)/2),
        width, height);
#endif
}

/* Pan the viewing window right - move image to the left and fill in
   the right-hand side */
static void pan_view_right(struct t_disp* pdisp)
{
    int move;

    move = MIN(HSCROLL, pdisp->width - pdisp->x - LCD_WIDTH);
    if (move > 0)
    {
        MYXLCD(scroll_left)(move); /* scroll left */
        pdisp->x += move;
        draw_image_rect(pdisp, LCD_WIDTH - move, 0, move, pdisp->height-pdisp->y);
        MYLCD_UPDATE();
    }
}

/* Pan the viewing window left - move image to the right and fill in
   the left-hand side */
static void pan_view_left(struct t_disp* pdisp)
{
    int move;

    move = MIN(HSCROLL, pdisp->x);
    if (move > 0)
    {
        MYXLCD(scroll_right)(move); /* scroll right */
        pdisp->x -= move;
        draw_image_rect(pdisp, 0, 0, move, pdisp->height-pdisp->y);
        MYLCD_UPDATE();
    }
}

/* Pan the viewing window up - move image down and fill in
   the top */
static void pan_view_up(struct t_disp* pdisp)
{
    int move;

    move = MIN(VSCROLL, pdisp->y);
    if (move > 0)
    {
        MYXLCD(scroll_down)(move); /* scroll down */
        pdisp->y -= move;
#ifdef HAVE_LCD_COLOR
        if (jpeg_settings.dither_mode == DITHER_DIFFUSION)
        {
            /* Draw over the band at the top of the last update
               caused by lack of error history on line zero. */
            move = MIN(move + 1, pdisp->y + pdisp->height);
        }
#endif
        draw_image_rect(pdisp, 0, 0, pdisp->width-pdisp->x, move);
        MYLCD_UPDATE();
    }
}

/* Pan the viewing window down - move image up and fill in
   the bottom */
static void pan_view_down(struct t_disp* pdisp)
{
    int move;

    move = MIN(VSCROLL, pdisp->height - pdisp->y - LCD_HEIGHT);
    if (move > 0)
    {
        MYXLCD(scroll_up)(move); /* scroll up */
        pdisp->y += move;
#ifdef HAVE_LCD_COLOR
        if (jpeg_settings.dither_mode == DITHER_DIFFUSION)
        {
            /* Save the line that was on the last line of the display
               and draw one extra line above then recover the line with
               image data that had an error history when it was drawn.
             */
            move++, pdisp->y--;
            rb->memcpy(rgb_linebuf,
                    rb->lcd_framebuffer + (LCD_HEIGHT - move)*LCD_WIDTH,
                    LCD_WIDTH*sizeof (fb_data));
        }
#endif

        draw_image_rect(pdisp, 0, LCD_HEIGHT - move, pdisp->width-pdisp->x, move);

#ifdef HAVE_LCD_COLOR
        if (jpeg_settings.dither_mode == DITHER_DIFFUSION)
        {
            /* Cover the first row drawn with previous image data. */
            rb->memcpy(rb->lcd_framebuffer + (LCD_HEIGHT - move)*LCD_WIDTH,
                        rgb_linebuf, LCD_WIDTH*sizeof (fb_data));
            pdisp->y++;
        }
#endif
        MYLCD_UPDATE();
    }
}

/* interactively scroll around the image */
int scroll_bmp(struct t_disp* pdisp)
{
    int button;
    int lastbutton = 0;

    while (true)
    {
        if (slideshow_enabled)
            button = rb->button_get_w_tmo(jpeg_settings.ss_timeout * HZ);
        else
            button = rb->button_get(true);

        running_slideshow = false;

        switch(button)
        {
        case JPEG_LEFT:
            if (entries > 1 && pdisp->width <= LCD_WIDTH
                            && pdisp->height <= LCD_HEIGHT)
                return change_filename(DIR_PREV);
        case JPEG_LEFT | BUTTON_REPEAT:
            pan_view_left(pdisp);
            break;

        case JPEG_RIGHT:
            if (entries > 1 && pdisp->width <= LCD_WIDTH
                            && pdisp->height <= LCD_HEIGHT)
                return change_filename(DIR_NEXT);
        case JPEG_RIGHT | BUTTON_REPEAT:
            pan_view_right(pdisp);
            break;

        case JPEG_UP:
        case JPEG_UP | BUTTON_REPEAT:
            pan_view_up(pdisp);
            break;

        case JPEG_DOWN:
        case JPEG_DOWN | BUTTON_REPEAT:
            pan_view_down(pdisp);
            break;

        case BUTTON_NONE:
            if (!slideshow_enabled)
                break;
            running_slideshow = true;
            if (entries > 1)
                return change_filename(DIR_NEXT);
            break;

#ifdef JPEG_SLIDE_SHOW
        case JPEG_SLIDE_SHOW:
            slideshow_enabled = !slideshow_enabled;
            running_slideshow = slideshow_enabled;
            break;
#endif

#ifdef JPEG_NEXT_REPEAT
        case JPEG_NEXT_REPEAT:
#endif
        case JPEG_NEXT:
            if (entries > 1)
                return change_filename(DIR_NEXT);
            break;

#ifdef JPEG_PREVIOUS_REPEAT
        case JPEG_PREVIOUS_REPEAT:
#endif
        case JPEG_PREVIOUS:
            if (entries > 1)
                return change_filename(DIR_PREV);
            break;

        case JPEG_ZOOM_IN:
#ifdef JPEG_ZOOM_PRE
            if (lastbutton != JPEG_ZOOM_PRE)
                break;
#endif
            return ZOOM_IN;
            break;

        case JPEG_ZOOM_OUT:
#ifdef JPEG_ZOOM_PRE
            if (lastbutton != JPEG_ZOOM_PRE)
                break;
#endif
            return ZOOM_OUT;
            break;
#ifdef JPEG_RC_MENU
        case JPEG_RC_MENU:
#endif
        case JPEG_MENU:
#ifdef USEGSLIB
            grey_show(false); /* switch off greyscale overlay */
#endif
            if (show_menu() == 1)
                return PLUGIN_OK;

#ifdef USEGSLIB
            grey_show(true); /* switch on greyscale overlay */
#else
            draw_image_rect(pdisp, 0, 0,
                            pdisp->width-pdisp->x, pdisp->height-pdisp->y);
            MYLCD_UPDATE();
#endif
            break;
        default:
            if (rb->default_event_handler_ex(button, cleanup, NULL)
                == SYS_USB_CONNECTED)
                return PLUGIN_USB_CONNECTED;
            break;

        } /* switch */

        if (button != BUTTON_NONE)
            lastbutton = button;
    } /* while (true) */
}

/********************* main function *************************/

/* callback updating a progress meter while JPEG decoding */
void cb_progress(int current, int total)
{
    rb->yield(); /* be nice to the other threads */
    if(!running_slideshow)
    {
        rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],
                            0, LCD_HEIGHT-8, LCD_WIDTH, 8,
                            total, 0, current, HORIZONTAL);
        rb->lcd_update_rect(0, LCD_HEIGHT-8, LCD_WIDTH, 8);
    }
#ifndef USEGSLIB
    else
    {
        /* in slideshow mode, keep gui interference to a minimum */
        rb->gui_scrollbar_draw(rb->screens[SCREEN_MAIN],
                            0, LCD_HEIGHT-4, LCD_WIDTH, 4,
                            total, 0, current, HORIZONTAL);
        rb->lcd_update_rect(0, LCD_HEIGHT-4, LCD_WIDTH, 4);
    }
#endif
}

int jpegmem(struct jpeg *p_jpg, int ds)
{
    int size;

    size = (p_jpg->x_phys/ds/p_jpg->subsample_x[0])
         * (p_jpg->y_phys/ds/p_jpg->subsample_y[0]);
#ifdef HAVE_LCD_COLOR
    if (p_jpg->blocks > 1) /* colour, add requirements for chroma */
    {
        size += (p_jpg->x_phys/ds/p_jpg->subsample_x[1])
              * (p_jpg->y_phys/ds/p_jpg->subsample_y[1]);
        size += (p_jpg->x_phys/ds/p_jpg->subsample_x[2])
              * (p_jpg->y_phys/ds/p_jpg->subsample_y[2]);
    }
#endif
    return size;
}

/* how far can we zoom in without running out of memory */
int min_downscale(struct jpeg *p_jpg, int bufsize)
{
    int downscale = 8;

    if (jpegmem(p_jpg, 8) > bufsize)
        return 0; /* error, too large, even 1:8 doesn't fit */

    while (downscale > 1 && jpegmem(p_jpg, downscale/2) <= bufsize)
        downscale /= 2;

    return downscale;
}

/* how far can we zoom out, to fit image into the LCD */
int max_downscale(struct jpeg *p_jpg)
{
    int downscale = 1;

    while (downscale < 8 && (p_jpg->x_size/downscale > LCD_WIDTH
                          || p_jpg->y_size/downscale > LCD_HEIGHT))
    {
        downscale *= 2;
    }

    return downscale;
}

/* load image from filename. */
int load_image(char* filename, struct jpeg *p_jpg)
{
    int fd;
    int filesize;
    unsigned char* buf_jpeg; /* compressed JPEG image */
    int status;

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        rb->splashf(HZ, "err opening %s:%d", filename, fd);
        return PLUGIN_ERROR;
    }
    filesize = rb->filesize(fd);

    /* allocate JPEG buffer */
    buf_jpeg = buf;

    /* we can start the decompressed images behind it */
    buf_images = buf_root = buf + filesize;
    buf_images_size = root_size = buf_size - filesize;

    if (buf_images_size <= 0)
    {
        rb->close(fd);
        return PLUGIN_OUTOFMEM;
    }

    if(!running_slideshow)
    {
        rb->snprintf(print, sizeof(print), "%s:", rb->strrchr(filename,'/')+1);
        rb->lcd_puts(0, 0, print);
        rb->lcd_update();

        rb->snprintf(print, sizeof(print), "loading %d bytes", filesize);
        rb->lcd_puts(0, 1, print);
        rb->lcd_update();
    }

    rb->read(fd, buf_jpeg, filesize);
    rb->close(fd);

    if(!running_slideshow)
    {
        rb->snprintf(print, sizeof(print), "decoding markers");
        rb->lcd_puts(0, 2, print);
        rb->lcd_update();
    }
#ifndef SIMULATOR
    else if(immediate_ata_off)
    {
        /* running slideshow and time is long enough: power down disk */
        rb->storage_sleep();
    }
#endif

    /* process markers, unstuffing */
    status = process_markers(buf_jpeg, filesize, p_jpg);

    if (status < 0 || (status & (DQT | SOF0)) != (DQT | SOF0))
    {   /* bad format or minimum components not contained */
        rb->splashf(HZ, "unsupported %d", status);
        return PLUGIN_ERROR;
    }

    if (!(status & DHT)) /* if no Huffman table present: */
        default_huff_tbl(p_jpg); /* use default */
    build_lut(p_jpg); /* derive Huffman and other lookup-tables */

    if(!running_slideshow)
    {
        rb->snprintf(print, sizeof(print), "image %dx%d",
                            p_jpg->x_size, p_jpg->y_size);
        rb->lcd_puts(0, 2, print);
        rb->lcd_update();
    }

    return PLUGIN_OK;
}

/* return decoded or cached image */
struct t_disp* get_image(struct jpeg* p_jpg, int ds)
{
    int w, h; /* used to center output */
    int size; /* decompressed image size */
    long time; /* measured ticks */
    int status;

    struct t_disp* p_disp = &disp[ds]; /* short cut */

    if (p_disp->bitmap[0] != NULL)
    {
        return p_disp; /* we still have it */
    }

    /* assign image buffer */

    /* physical size needed for decoding */
    size = jpegmem(p_jpg, ds);
    if (buf_images_size <= size)
    {   /* have to discard the current */
        int i;
        for (i=1; i<=8; i++)
            disp[i].bitmap[0] = NULL; /* invalidate all bitmaps */
        buf_images = buf_root; /* start again from the beginning of the buffer */
        buf_images_size = root_size;
    }

#ifdef HAVE_LCD_COLOR
    if (p_jpg->blocks > 1) /* colour jpeg */
    {
        int i;

        for (i = 1; i < 3; i++)
        {
            size = (p_jpg->x_phys / ds / p_jpg->subsample_x[i])
                 * (p_jpg->y_phys / ds / p_jpg->subsample_y[i]);
            p_disp->bitmap[i] = buf_images;
            buf_images += size;
            buf_images_size -= size;
        }
        p_disp->csub_x = p_jpg->subsample_x[1];
        p_disp->csub_y = p_jpg->subsample_y[1];
    }
    else
    {
        p_disp->csub_x = p_disp->csub_y = 0;
        p_disp->bitmap[1] = p_disp->bitmap[2] = buf_images;
    }
#endif
    /* size may be less when decoded (if height is not block aligned) */
    size = (p_jpg->x_phys/ds) * (p_jpg->y_size / ds);
    p_disp->bitmap[0] = buf_images;
    buf_images += size;
    buf_images_size -= size;

    if(!running_slideshow)
    {
        rb->snprintf(print, sizeof(print), "decoding %d*%d",
            p_jpg->x_size/ds, p_jpg->y_size/ds);
        rb->lcd_puts(0, 3, print);
        rb->lcd_update();
    }

    /* update image properties */
    p_disp->width = p_jpg->x_size / ds;
    p_disp->stride = p_jpg->x_phys / ds; /* use physical size for stride */
    p_disp->height = p_jpg->y_size / ds;

    /* the actual decoding */
    time = *rb->current_tick;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
    status = jpeg_decode(p_jpg, p_disp->bitmap, ds, cb_progress);
    rb->cpu_boost(false);
#else
    status = jpeg_decode(p_jpg, p_disp->bitmap, ds, cb_progress);
#endif
    if (status)
    {
        rb->splashf(HZ, "decode error %d", status);
        return NULL;
    }
    time = *rb->current_tick - time;

    if(!running_slideshow)
    {
        rb->snprintf(print, sizeof(print), " %ld.%02ld sec ", time/HZ, time%HZ);
        rb->lcd_getstringsize(print, &w, &h); /* centered in progress bar */
        rb->lcd_putsxy((LCD_WIDTH - w)/2, LCD_HEIGHT - h, print);
        rb->lcd_update();
    }

    return p_disp;
}


/* set the view to the given center point, limit if necessary */
void set_view (struct t_disp* p_disp, int cx, int cy)
{
    int x, y;

    /* plain center to available width/height */
    x = cx - MIN(LCD_WIDTH, p_disp->width) / 2;
    y = cy - MIN(LCD_HEIGHT, p_disp->height) / 2;

    /* limit against upper image size */
    x = MIN(p_disp->width - LCD_WIDTH, x);
    y = MIN(p_disp->height - LCD_HEIGHT, y);

    /* limit against negative side */
    x = MAX(0, x);
    y = MAX(0, y);

    p_disp->x = x; /* set the values */
    p_disp->y = y;
}

/* calculate the view center based on the bitmap position */
void get_view(struct t_disp* p_disp, int* p_cx, int* p_cy)
{
    *p_cx = p_disp->x + MIN(LCD_WIDTH, p_disp->width) / 2;
    *p_cy = p_disp->y + MIN(LCD_HEIGHT, p_disp->height) / 2;
}

/* load, decode, display the image */
int load_and_show(char* filename)
{
    int status;
    struct t_disp* p_disp; /* currenly displayed image */
    int cx, cy; /* view center */

#if LCD_DEPTH > 1
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_backdrop(NULL);
#endif
    rb->lcd_clear_display();

    rb->memset(&disp, 0, sizeof(disp));
    rb->memset(&jpg, 0, sizeof(jpg)); /* clear info struct */

    if (rb->button_get(false) == JPEG_MENU)
        status = PLUGIN_ABORT;
    else
        status = load_image(filename, &jpg);

    if (status == PLUGIN_OUTOFMEM)
    {
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
        if(plug_buf)
        {
            rb->lcd_setfont(FONT_SYSFIXED);
            rb->lcd_clear_display();
            rb->snprintf(print,sizeof(print),"%s:",rb->strrchr(filename,'/')+1);
            rb->lcd_puts(0,0,print);
            rb->lcd_puts(0,1,"Not enough plugin memory!");
            rb->lcd_puts(0,2,"Zoom In: Stop playback.");
            if(entries>1)
                rb->lcd_puts(0,3,"Left/Right: Skip File.");
            rb->lcd_puts(0,4,"Show Menu: Quit.");
            rb->lcd_update();
            rb->lcd_setfont(FONT_UI);

            rb->button_clear_queue();

            while (1)
            {
                int button = rb->button_get(true);
                switch(button)
                {
                    case JPEG_ZOOM_IN:
                        plug_buf = false;
                        buf = rb->plugin_get_audio_buffer((size_t *)&buf_size);
                        /*try again this file, now using the audio buffer */
                        return PLUGIN_OTHER;
#ifdef JPEG_RC_MENU
                    case JPEG_RC_MENU:
#endif
                    case JPEG_MENU:
                        return PLUGIN_OK;

                    case JPEG_LEFT:
                        if(entries>1)
                        {
                            rb->lcd_clear_display();
                            return change_filename(DIR_PREV);
                        }
                        break;

                    case JPEG_RIGHT:
                        if(entries>1)
                        {
                            rb->lcd_clear_display();
                            return change_filename(DIR_NEXT);
                        }
                        break;
                    default:
                        if(rb->default_event_handler_ex(button, cleanup, NULL)
                                == SYS_USB_CONNECTED)
                            return PLUGIN_USB_CONNECTED;

                }
            }
        }
        else
#endif
        {
            rb->splash(HZ, "Out of Memory");
            file_pt[curfile] = NULL;
            return change_filename(direction);
        }
    }
    else if (status == PLUGIN_ERROR)
    {
        file_pt[curfile] = NULL;
        return change_filename(direction);
    }
    else if (status == PLUGIN_ABORT) {
        rb->splash(HZ, "aborted");
        return PLUGIN_OK;
    }

    ds_max = max_downscale(&jpg);            /* check display constraint */
    ds_min = min_downscale(&jpg, buf_images_size); /* check memory constraint */
    if (ds_min == 0)
    {
        rb->splash(HZ, "too large");
        file_pt[curfile] = NULL;
        return change_filename(direction);
    }
    else if (ds_max < ds_min)
        ds_max = ds_min;

    ds = ds_max; /* initialize setting */
    cx = jpg.x_size/ds/2; /* center the view */
    cy = jpg.y_size/ds/2;

    do  /* loop the image prepare and decoding when zoomed */
    {
        p_disp = get_image(&jpg, ds); /* decode or fetch from cache */
        if (p_disp == NULL)
        {
            file_pt[curfile] = NULL;
            return change_filename(direction);
        }

        set_view(p_disp, cx, cy);

        if(!running_slideshow)
        {
            rb->snprintf(print, sizeof(print), "showing %dx%d",
                p_disp->width, p_disp->height);
            rb->lcd_puts(0, 3, print);
            rb->lcd_update();
        }

        MYLCD(clear_display)();
        draw_image_rect(p_disp, 0, 0,
                        p_disp->width-p_disp->x, p_disp->height-p_disp->y);
        MYLCD_UPDATE();

#ifdef USEGSLIB
        grey_show(true); /* switch on greyscale overlay */
#endif

        /* drawing is now finished, play around with scrolling
         * until you press OFF or connect USB
         */
        while (1)
        {
            status = scroll_bmp(p_disp);
            if (status == ZOOM_IN)
            {
                if (ds > ds_min)
                {
                    ds /= 2; /* reduce downscaling to zoom in */
                    get_view(p_disp, &cx, &cy);
                    cx *= 2; /* prepare the position in the new image */
                    cy *= 2;
                }
                else
                    continue;
            }

            if (status == ZOOM_OUT)
            {
                if (ds < ds_max)
                {
                    ds *= 2; /* increase downscaling to zoom out */
                    get_view(p_disp, &cx, &cy);
                    cx /= 2; /* prepare the position in the new image */
                    cy /= 2;
                }
                else
                    continue;
            }
            break;
        }

#ifdef USEGSLIB
        grey_show(false); /* switch off overlay */
#endif
        rb->lcd_clear_display();
    }
    while (status != PLUGIN_OK && status != PLUGIN_USB_CONNECTED
            && status != PLUGIN_OTHER);
#ifdef USEGSLIB
    rb->lcd_update();
#endif
    return status;
}

/******************** Plugin entry point *********************/

enum plugin_status plugin_start(const void* parameter)
{
    int condition;
#ifdef USEGSLIB
    long greysize; /* helper */
#endif
#if LCD_DEPTH > 1
    old_backdrop = rb->lcd_get_backdrop();
#endif

    if(!parameter) return PLUGIN_ERROR;

#if PLUGIN_BUFFER_SIZE >= MIN_MEM
    buf = rb->plugin_get_buffer((size_t *)&buf_size);
#else
    buf = rb->plugin_get_audio_buffer((size_t *)&buf_size);
#endif

    rb->strcpy(np_file, parameter);
    get_pic_list();

    if(!entries) return PLUGIN_ERROR;

#if (PLUGIN_BUFFER_SIZE >= MIN_MEM) && !defined(SIMULATOR)
    if(!rb->audio_status())
    {
        plug_buf = false;
        buf = rb->plugin_get_audio_buffer((size_t *)&buf_size);
    }
#endif

#ifdef USEGSLIB
    if (!grey_init(buf, buf_size, GREY_ON_COP,
                   LCD_WIDTH, LCD_HEIGHT, &greysize))
    {
        rb->splash(HZ, "grey buf error");
        return PLUGIN_ERROR;
    }
    buf += greysize;
    buf_size -= greysize;
#endif

    /* should be ok to just load settings since the plugin itself has
       just been loaded from disk and the drive should be spinning */
    configfile_load(JPEG_CONFIGFILE, jpeg_config,
                    ARRAYLEN(jpeg_config), JPEG_SETTINGS_MINVERSION);
    old_settings = jpeg_settings;

    /* Turn off backlight timeout */
    backlight_force_on(); /* backlight control in lib/helper.c */

    do
    {
        condition = load_and_show(np_file);
    } while (condition != PLUGIN_OK && condition != PLUGIN_USB_CONNECTED
            && condition != PLUGIN_ERROR);

    if (rb->memcmp(&jpeg_settings, &old_settings, sizeof (jpeg_settings)))
    {
        /* Just in case drive has to spin, keep it from looking locked */
        rb->splash(0, "Saving Settings");
        configfile_save(JPEG_CONFIGFILE, jpeg_config,
                        ARRAYLEN(jpeg_config), JPEG_SETTINGS_VERSION);
    }

#if !defined(SIMULATOR) && defined(HAVE_DISK_STORAGE)
    /* set back ata spindown time in case we changed it */
    rb->storage_spindown(rb->global_settings->disk_spindown);
#endif

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(); /* backlight control in lib/helper.c */

#ifdef USEGSLIB
    grey_release(); /* deinitialize */
#endif

    return condition;
}
