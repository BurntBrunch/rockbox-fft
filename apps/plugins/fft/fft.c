/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Delyan Kratunov
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

#include "lib/helper.h"
#include "math.h"

PLUGIN_HEADER

#if CONFIG_KEYPAD == RECORDER_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#   define MINESWP_SCROLLWHEEL
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH      BUTTON_RIGHT
#   define FFT_SCALE         BUTTON_MENU
#   define FFT_COLOR       BUTTON_PLAY
#   define FFT_QUIT       (BUTTON_SELECT | BUTTON_MENU)

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)

#   define MINESWP_SCROLLWHEEL
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)

#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_C200_PAD) || \
(CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
(CONFIG_KEYPAD == SANSA_M200_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH      BUTTON_RIGHT
#   define FFT_SCALE         BUTTON_UP
#   define FFT_COLOR       BUTTON_DOWN
#   define FFT_QUIT       BUTTON_POWER

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_SCROLL_UP
#   define FFT_COLOR     BUTTON_SCROLL_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_BACK

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#   define FFT_PREV_GRAPH     BUTTON_RC_REW
#   define FFT_NEXT_GRAPH    BUTTON_RC_FF
#   define FFT_SCALE       BUTTON_RC_VOL_UP
#   define FFT_COLOR     BUTTON_RC_VOL_DOWN
#   define FFT_QUIT     BUTTON_RC_REC

#elif (CONFIG_KEYPAD == COWOND2_PAD)
#   define FFT_QUIT     BUTTON_POWER

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_BACK

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#else
#error No keymap defined!
#endif

#define FFT_SIZE (1024)

#ifdef FFTR
#	include "kiss_fftr.h"
#else
#	include "kiss_fft.h"
#endif

#include "_kiss_fft_guts.h" /* sizeof(struct kiss_fft_state) */

#ifdef FFTR /* Real numbers FFT */
#   define ARRAYSIZE_IN FFT_SIZE
#   define ARRAYSIZE_OUT (FFT_SIZE/2)
#	define ARRAYSIZE_PLOT ARRAYSIZE_OUT

#   define BUFSIZE (sizeof(struct kiss_fftr_state) + \
(sizeof(struct kiss_fft_state) + sizeof(kiss_fft_cpx)*(FFT_SIZE-1)) + \
sizeof(kiss_fft_cpx) * ( FFT_SIZE * 3 / 2))

#define FFT_ALLOC kiss_fftr_alloc
#define FFT_FFT   kiss_fftr
#define FFT_CFG   kiss_fftr_cfg

#else /* Normal FFT */
#   define ARRAYSIZE_IN FFT_SIZE
#   define ARRAYSIZE_OUT FFT_SIZE
#	define ARRAYSIZE_PLOT FFT_SIZE/2
#   define BUFSIZE (sizeof(struct kiss_fft_state) + \
sizeof(kiss_fft_cpx)*(FFT_SIZE-1))
#define FFT_ALLOC kiss_fft_alloc
#define FFT_FFT   kiss_fft
#define FFT_CFG   kiss_fft_cfg
#endif

#ifdef FFTR
static kiss_fft_scalar input[ARRAYSIZE_IN];
#else
static kiss_fft_cpx input[ARRAYSIZE_IN];
#endif

static kiss_fft_cpx output[ARRAYSIZE_OUT];
static int32_t plot[ARRAYSIZE_PLOT];
static char buffer[BUFSIZE];

/************************* Math functions *************************/

#define LOGE_TEN 2.302585093
#define QLOGE_TEN float_q(LOGE_TEN, 16)

/* Returns logarithmically scaled values in S15.16 format */
int32_t get_log_value(int32_t value, bool fraction)
{
    int32_t result;

    /* ln (value) / ln (10) = log10 (value)*/
    result = Q_DIV(flog((fraction ? value : value << 16)), QLOGE_TEN, 16);

    /* 10 * log10 (value)*/
    result = Q_MUL(result, 10 << 16, 16);

    return result;
}

void apply_window_func(void)
{
    const int32_t hamming_a = float_q(0.53836, 16), hamming_b =
            float_q(0.46164, 16);
    size_t i;
    for (i = 0; i < ARRAYSIZE_IN; ++i)
    {
        int32_t cos;
        (void) fsincos(Q_DIV(i << 16, (ARRAYSIZE_IN - 1) << 16, 16), &cos);
        cos >>= 15;

        /* value = value * (hamming_a - hamming_b * cos( 2 * pi * i/(ArraySize - 1) ) ) */
        input[i].r
                = Q_MUL(input[i].r << 16, ( hamming_a - Q_MUL(cos, hamming_b, 16) ), 16)
                        >> 16;
        input[i].i
                = Q_MUL(input[i].i << 16, ( hamming_a - Q_MUL(cos, hamming_b, 16) ), 16)
                        >> 16;
    }
}

/* Calculates the magnitudes from complex numbers and returns the maximum */
int32_t calc_magnitudes(bool logarithmic)
{
    int32_t tmp;
    size_t i;

    int32_t max = -2147483647;

    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        tmp = Q_MUL( ((int32_t) output[i].r) << 16,
                     ((int32_t) output[i].r) << 16, 16);
        tmp += Q_MUL( ((int32_t) output[i].i) << 16,
                      ((int32_t) output[i].i) << 16, 16);

        tmp = fsqrt(tmp, 16);

        if (logarithmic)
            tmp = get_log_value(tmp, true);

        if (logarithmic)
            plot[i] = tmp;
        else
            plot[i] = tmp >> 16;

        if (plot[i] > max)
            max = plot[i];
    }
    return max;
}
/************************ End of math functions ***********************/

/********************* Plotting functions (modes) *********************/

const unsigned char* modes_text[] = { "Lines", "Bars" };
const unsigned char* scales_text[] = { "Linear scale", "Logarithmic scale" };
#define MODES_COUNT 2
#define REFRESH_RATE 20

struct {
    bool logarithmic;
    bool colored;
} graph_settings;

static long next_update = 0;

void draw_lines(void);
void draw_bars(void);

void draw(char mode, const unsigned char* message)
{
    static uint32_t show_message = 0;
    static unsigned char* last_message = 0;
    if (message != 0)
    {
        last_message = (unsigned char*) message;
        show_message = 5;
    }

    rb->lcd_clear_display();
    switch (mode)
    {
        default:
        case 0: {
            draw_lines();
            break;
        }
        case 1: {
            draw_bars();
            break;
        }
    }

    if (show_message > 0)
    {
        int x, y;
        rb->lcd_getstringsize(last_message, &x, &y);
        x += 6; /* 3 px of horizontal padding and */
        y += 4; /* 2 px of vertical padding */

        rb->lcd_set_foreground(LCD_DARKGRAY);
        rb->lcd_set_background(LCD_DARKGRAY);
        rb->lcd_fillrect(LCD_WIDTH - x, 0, LCD_WIDTH, y);

        rb->lcd_set_foreground(LCD_DEFAULT_FG);
        rb->lcd_putsxy(LCD_WIDTH - x + 3, 2, last_message);
        rb->lcd_set_background(LCD_DEFAULT_BG);

        show_message--;
    }
    rb->lcd_update();

    /* we still have time in our time slot, so we sleep() */
    if (*rb->current_tick < next_update)
        rb->sleep(next_update - *rb->current_tick);

    /* end of next time slot */
    next_update = *rb->current_tick + HZ / REFRESH_RATE;
}

void draw_lines(void)
{
    static int max = 0;
    static bool last_mode = false;

    static const int32_t hfactor =
            Q15_DIV(LCD_WIDTH << 15, (ARRAYSIZE_PLOT) << 15),
            bins_per_pixel = (ARRAYSIZE_PLOT) / LCD_WIDTH;

    if (graph_settings.logarithmic != last_mode)
    {
        max = 0; /* reset the graph on scaling mode change */
        last_mode = graph_settings.logarithmic;
    }
    int32_t new_max = calc_magnitudes(graph_settings.logarithmic);

    if (new_max > max)
        max = new_max;

    if (new_max == 0 || max == 0) /* nothing to draw */
        return;

    int32_t vfactor;

    if (graph_settings.logarithmic)
        vfactor = Q_DIV(LCD_HEIGHT << 16, max, 16); /* s15.16 */
    else
        vfactor = Q15_DIV(LCD_HEIGHT << 15, max << 15); /* s16.15 */

    int32_t i;

    /* take the average of neighboring bins
     * if we have to scale the graph horizontally */
    int64_t bins_avg = 0;
    bool draw = true;
    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        int32_t x = 0, y = 0;

        x = Q15_MUL(hfactor, i << 15) >> 15;

        if (hfactor < 32768) /* hfactor < 0, graph compression */
        {
            draw = false;
            bins_avg += plot[i];

            /* fix the division by zero warning:
             * bins_per_pixel is zero when the graph is expanding;
             * execution won't even reach this point - this is a dummy constant
             */
            const int32_t div = bins_per_pixel > 0 ? bins_per_pixel : 1;
            if ((i + 1) % div == 0)
            {
                if (graph_settings.logarithmic)
                {
                    bins_avg = Q_DIV(bins_avg, div << 16, 16);

                    y = Q_MUL(vfactor, bins_avg, 16) >> 16;
                }
                else
                {
                    bins_avg = Q15_DIV(bins_avg << 15, div << 15);
                    bins_avg += (1 << 14); /* rounding up */

                    y = Q15_MUL(vfactor, bins_avg) >> 15;
                }

                bins_avg = 0;
                draw = true;
            }
        }
        else
        {
            y = Q15_MUL(vfactor, plot[i] << 15) >> 15;
            draw = true;
        }

        if (draw)
            rb->lcd_drawline(x, LCD_HEIGHT, x, LCD_HEIGHT - y);
    }
}

void draw_bars(void)
{
    static const unsigned int bars = 12, border = 3, items = ARRAYSIZE_PLOT
            / bars, width = (LCD_WIDTH - ((bars - 1) * border)) / bars;

    calc_magnitudes(graph_settings.logarithmic);

    int64_t bars_values[bars], bars_max = 0, avg = 0;
    unsigned int i, bars_idx = 0;
    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        avg += plot[i];
        if ((i + 1) % items == 0)
        {
            /* Calculate the average value and keep the fractional part
             * for some added precision */
            if (graph_settings.logarithmic)
                avg = Q_DIV(avg, items << 16, 16); /* s15.16 */
            else
                avg = Q15_DIV(avg << 15, items << 15); /* s16.15 */
            bars_values[bars_idx] = avg;

            if (bars_values[bars_idx] > bars_max)
                bars_max = bars_values[bars_idx];

            bars_idx++;
            avg = 0;
        }
    }

    if(bars_max == 0) /* nothing to draw */
        return;

    /* Give the graph some headroom */
    bars_max = Q15_MUL(bars_max, float_q15(1.1));

    int64_t vfactor;
    if (graph_settings.logarithmic)
        vfactor = Q_DIV(LCD_HEIGHT << 16, bars_max, 16);
    else
        vfactor = Q15_DIV(LCD_HEIGHT << 15, bars_max);

    for (i = 0; i < bars; ++i)
    {
        int x = (i) * (border + width);
        int y;
        if (graph_settings.logarithmic)
        {
            y = Q_MUL(vfactor, bars_values[i], 16);
            y += (1 << 15);
            y >>= 16;
        }
        else
        {
            y = Q15_MUL(vfactor, bars_values[i]);
            y += (1 << 14);
            y >>= 15;
        }

        rb->lcd_fillrect(x, LCD_HEIGHT - y, width, y);
    }
}

/********************* End of plotting functions (modes) *********************/

enum plugin_status plugin_start(const void* parameter)
{
    (void) parameter;
    if ((rb->audio_status() & AUDIO_STATUS_PLAY) == 0)
    {
        rb->splash(HZ * 2, "No track playing. Exiting..");
        return PLUGIN_OK;
    }

    rb->lcd_set_backdrop(NULL);
    backlight_force_on();

    /* Defaults; TODO: mode should be a saveable setting */
    bool run = true;
    int mode = 0;
    graph_settings.logarithmic = true;
    graph_settings.colored = false; /* doesn't do anything yet*/

    /* set the end of the first time slot - rest of the
     * next_update work is done in draw() */
    next_update = *rb->current_tick + HZ / REFRESH_RATE;

    size_t size = sizeof(buffer);
    FFT_CFG state = FFT_ALLOC(FFT_SIZE, 0, buffer, &size);

    if (state == 0)
    {
        DEBUGF("needed data: %i", (int) size);
        return PLUGIN_ERROR;
    }

    /* taken out of the main loop for efficiency (?)*/
    kiss_fft_scalar left, right;
    kiss_fft_scalar* value;
    int count;

    while (run)
    {
        value = (kiss_fft_scalar*) rb->pcm_get_peak_buffer(&count);
        if (value == 0 || count == 0)
        {
            rb->yield();
            continue;
        }

        int idx = 0; /* offset in the buffer */
        int fft_idx = 0; /* offset in input */

        do
        {
            left = *(value + idx);
            idx += 2;

            right = *(value + idx);
            idx += 2;
#ifdef FFTR
            input[fft_idx] = left/2 + right/2;
#else
            input[fft_idx].r = left;
            input[fft_idx].i = right;
#endif
            fft_idx++;

            if (fft_idx == ARRAYSIZE_IN)
                break;
        } while (idx < count);

        if (fft_idx == ARRAYSIZE_IN)
        {
            apply_window_func();

            /* Play nice - the sleep at the end of draw()
             * only tries to maintain the frame rate */
            rb->yield();
            FFT_FFT(state, input, output);
            draw(mode, 0);

            fft_idx = 0;
        };

        int button = rb->button_get(false);
        switch (button)
        {
            case FFT_QUIT:
                run = false;
                break;
            case FFT_PREV_GRAPH: {
                mode -= 1;
                if (mode < 0)
                    mode = MODES_COUNT-1;
                draw(mode, modes_text[(int) mode]);
                break;
            }
            case FFT_NEXT_GRAPH: {
                mode += 1;
                if (mode >= MODES_COUNT)
                    mode = 0;
                draw(mode, modes_text[(int) mode]);
                break;
            }
            case FFT_SCALE: {
                graph_settings.logarithmic = !graph_settings.logarithmic;
                draw(mode, scales_text[graph_settings.logarithmic ? 1 : 0]);
                break;
            }
        }
    }
    backlight_use_settings();
    return PLUGIN_OK;
}
