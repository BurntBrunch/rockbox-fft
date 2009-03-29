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
#define FIXED_POINT 16
#include "kiss_fftr.h"
#include "_kiss_fft_guts.h" /* sizeof(struct kiss_fft_state) */

#include "lib/helper.h"
#include "math.h"

PLUGIN_HEADER


/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define FFT_QUIT BUTTON_OFF
#define FFT_MODE BUTTON_F1
#define FFT_RESTART BUTTON_PLAY

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define FFT_QUIT BUTTON_OFF
#define FFT_MODE BUTTON_F1
#define FFT_RESTART BUTTON_SELECT

#elif CONFIG_KEYPAD == PLAYER_PAD
#define FFT_QUIT BUTTON_STOP
#define FFT_MODE BUTTON_MENU
#define FFT_RESTART BUTTON_PLAY

#elif CONFIG_KEYPAD == ONDIO_PAD
#define FFT_QUIT BUTTON_OFF
#define FFT_MODE BUTTON_MENU
#define FFT_RESTART BUTTON_RIGHT

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define FFT_QUIT BUTTON_OFF
#define FFT_MODE BUTTON_MODE
#define FFT_RESTART BUTTON_ON

#define FFT_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define FFT_QUIT BUTTON_MENU
#define FFT_MODE BUTTON_SELECT
#define FFT_RESTART BUTTON_PLAY

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define FFT_QUIT BUTTON_PLAY
#define FFT_MODE BUTTON_MODE
#define FFT_RESTART BUTTON_SELECT

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define FFT_QUIT BUTTON_POWER
#define FFT_MODE BUTTON_SELECT
#define FFT_RESTART BUTTON_PLAY

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define FFT_QUIT BUTTON_POWER
#define FFT_MODE BUTTON_A
#define FFT_RESTART BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
      (CONFIG_KEYPAD == SANSA_C200_PAD) || \
      (CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
      (CONFIG_KEYPAD == SANSA_M200_PAD) || \
      (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define FFT_QUIT BUTTON_POWER
#define FFT_MODE BUTTON_DOWN
#define FFT_RESTART BUTTON_SELECT

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define FFT_QUIT BUTTON_POWER
#define FFT_MODE BUTTON_FF
#define FFT_RESTART BUTTON_PLAY

#elif CONFIG_KEYPAD == MROBE500_PAD
#define FFT_QUIT BUTTON_POWER
#define FFT_MODE BUTTON_RC_FF
#define FFT_RESTART BUTTON_RC_PLAY

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define FFT_QUIT BUTTON_BACK
#define FFT_MODE BUTTON_SELECT
#define FFT_RESTART BUTTON_MENU

#elif CONFIG_KEYPAD == MROBE100_PAD
#define FFT_QUIT BUTTON_POWER
#define FFT_MODE BUTTON_DISPLAY
#define FFT_RESTART BUTTON_SELECT

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define FFT_QUIT BUTTON_RC_REC
#define FFT_MODE BUTTON_RC_MENU
#define FFT_RESTART BUTTON_RC_PLAY

#elif CONFIG_KEYPAD == COWOND2_PAD
#define FFT_QUIT BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define FFT_QUIT BUTTON_POWER
#define FFT_MODE BUTTON_PLAY
#define FFT_RESTART BUTTON_MENU

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define FFT_QUIT BUTTON_BACK
#define FFT_MODE BUTTON_SELECT
#define FFT_RESTART BUTTON_PLAY

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define FFT_QUIT BUTTON_POWER
#define FFT_MODE BUTTON_VIEW
#define FFT_RESTART BUTTON_MENU

#else
#error No keymap defined!
#endif

#define FFTR
#define FFT_SIZE 1024

#ifdef FFTR /* Real numbers FFT */
#   define ARRAYSIZE_IN FFT_SIZE
#   define ARRAYSIZE_OUT (FFT_SIZE/2)
#   define BUFSIZE (sizeof(struct kiss_fftr_state) + \
(sizeof(struct kiss_fft_state) + sizeof(kiss_fft_cpx)*(FFT_SIZE-1)) + \
sizeof(kiss_fft_cpx) * ( FFT_SIZE * 3 / 2))

#define fft_alloc kiss_fftr_alloc;
#define fft_fft   kiss_fftr;
typedef kiss_fftr_cfg   fft_cfg;

#else /* Normal FFT */
#   define ARRAYSIZE_IN FFT_SIZE
#   define ARRAYSIZE_OUT FFT_SIZE
#   define BUFSIZE (sizeof(struct kiss_fft_state) + \
sizeof(kiss_fft_cpx)*(FFT_SIZE-1))
#define fft_alloc kiss_fft_alloc;
#define fft_fft   kiss_fft;
typedef kiss_fft_cfg   fft_cfg;
#endif

#ifdef FFTR
    static kiss_fft_scalar data[ARRAYSIZE_IN];
#else
    static kiss_fft_cpx data[ARRAYSIZE_IN];
#endif

static kiss_fft_cpx fft[ARRAYSIZE_OUT];
static kiss_fft_scalar magnitudes[ARRAYSIZE_OUT];
static char mem[BUFSIZE];

/* Plotting functions (modes) */

#define MODES 2 /* 1 = Lines, 2 = Bars */
#define REFRESH_RATE 10

static long next_update = 0;

void draw_lines(void);
void draw_bars(void);

void draw(char mode)
{
	switch(mode)
	{
		case 1: draw_lines();break;
		case 2: draw_bars(); break;
		default: draw_lines();
	}

	/* we still have time in our time slot, so we sleep() */
	if (*rb->current_tick < next_update)
		rb->sleep( next_update - *rb->current_tick);

	/* end of next time slot */
	next_update = *rb->current_tick + HZ/REFRESH_RATE;
}

/* calculates the magnitudes from complex numbers and returns the maximum*/
int32_t calc_magnitudes(void)
{
	int32_t tmp;
	size_t i;

	int32_t max = 0;
	for(i=0; i<ARRAYSIZE_OUT; ++i)
	{
		tmp = Q_MUL( ((int32_t) fft[i].r) << 16, ((int32_t) fft[i].r) << 16, 16);
		tmp += Q_MUL(((int32_t) fft[i].i) << 16, ((int32_t) fft[i].i) << 16, 16);
		tmp = fsqrt(tmp, 13);
		magnitudes[i] = tmp >> 16;

		if (magnitudes[i] > max)
			max = magnitudes[i];
	}
	return max;
}

int32_t calc_real(void)
{
	size_t i;

	int32_t max = 0;
	for(i=0; i<ARRAYSIZE_OUT; ++i)
	{
		magnitudes[i] = fft[i].r;

		if (magnitudes[i] > max)
			max = magnitudes[i];
	}
	return max;
}


void draw_lines(void)
{
	static int max = 0;

	static const int32_t hfactor = Q15_DIV(LCD_WIDTH << 15, ARRAYSIZE_OUT << 15),
						 hfactorw = hfactor >> 15,
						 bins_per_pixel = ARRAYSIZE_OUT/LCD_WIDTH;

	int32_t new_max = calc_magnitudes();
	if(new_max > max)
		max = new_max;

	int32_t vfactor = Q15_DIV(LCD_HEIGHT << 15, max << 15);

	rb->lcd_clear_display();

	int32_t i;

	/* take the average of neighboring bins if we have to scale the graph horizontally */
	int32_t bins_avg=0; bool draw = true;
	for(i=0; i< ARRAYSIZE_OUT; ++i)
	{
		int32_t x=0, y=0;

		x = Q15_MUL(hfactor, i << 15) >> 15;

		if (hfactorw == 0 && bins_per_pixel > 0) /* graph compression */
		{
			draw = false;
			bins_avg += magnitudes[i];

			/* fix the division by zero warning:
			 * bins_per_pixel is zero when the graph is expanding,
			 * but won't even reach this point =/
			 */
			const int32_t div = bins_per_pixel > 0 ? bins_per_pixel : 1;
			if((i+1) % div == 0)
			{
				bins_avg = Q15_DIV(bins_avg << 15, div << 15);
				y = Q15_MUL(vfactor, bins_avg) >> 15;

				bins_avg = 0;
				draw = true;
			}
		}
		else
		{
			y = Q15_MUL(vfactor, magnitudes[i] << 15) >> 15;
			draw = true;
		}

		if(draw)
			rb->lcd_drawline(x, LCD_HEIGHT, x, LCD_HEIGHT-y);
	}
	rb->lcd_update();
}

void draw_bars(void)
{
	static const unsigned int bars = 8, border = 4, items = ARRAYSIZE_OUT/bars, width = (LCD_WIDTH - ((bars-1)*border)) / bars;

	calc_magnitudes();

	unsigned int bars_values[bars], bars_idx = 0, bars_max=0;
	unsigned int i, avg=0;
	for(i=0; i<ARRAYSIZE_OUT; ++i)
	{
		avg += magnitudes[i];
		if((i+1)%items == 0)
		{
			/* Calculate the average value and keep the fractional part
			 * for some added precision */
			avg = Q15_DIV(avg << 15, items << 15);
			bars_values[bars_idx] = avg;

			if(bars_values[bars_idx] > bars_max)
				bars_max = bars_values[bars_idx];

			bars_idx ++;
			avg = 0;
		}
	}

	/* Give the graph some headroom */
	bars_max = Q15_MUL(bars_max, float_q15(1.1));
	/* Round up */
	bars_max += 1 << 14;

	int32_t vfactor = Q15_DIV(LCD_HEIGHT << 15, bars_max);

	rb->lcd_clear_display();
	for(i=0; i<bars; ++i)
	{
		int x = (i)*(border + width);
		int y = Q15_MUL(vfactor, bars_values[i]) >> 15;
		rb->lcd_fillrect(x, LCD_HEIGHT-y, width, y);
	}

	rb->lcd_update();
}


enum plugin_status plugin_start(const void* parameter)
{
	(void) parameter;
	if((rb->audio_status() & AUDIO_STATUS_PLAY) == 0)
	{
		rb->splash(HZ*2, "No song playing. Exiting..");
		return PLUGIN_OK;
	}

	backlight_force_on();

	/* Defaults; TODO: mode should be a saveable setting */
	bool run = true;
	char mode = 1;

	/* set the end of the first time slot - rest of the
	 * next_update work is done in draw() */
	next_update = *rb->current_tick + HZ/REFRESH_RATE;

	size_t size = sizeof(mem);
	kiss_fftr_cfg state = kiss_fftr_alloc(FFT_SIZE, 0,mem,&size);

	if(state == 0)
	{
		DEBUGF("needed data: %i", (int)size);
		return PLUGIN_ERROR;
	}

	while(run)
	{
		kiss_fft_scalar left, right;
		int count;
		void* buffer = (void*) rb->pcm_get_peak_buffer(&count);

		if (buffer == 0 || count == 0)
		{
#ifdef SIMULATOR
			rb->sleep(HZ/500); /* 2ms - needed to make SDL happy */
#endif
		    continue;
	    }

        kiss_fft_scalar* value = (kiss_fft_scalar*) buffer;
        int idx=0; /* offset in the buffer */
        int fft_idx=0;

		do
		{
			left =  *(value+idx);
			idx += 2;

			right = *(value+idx);
			idx += 2;
#ifdef FFTR
			data[fft_idx] = left/2 + right/2;
#else
			data[fft_idx].r = left;
			data[fft_idx].i = right;
#endif
    		fft_idx++;

    		if(fft_idx == ARRAYSIZE_IN)
		        break;
		}while(idx < count);

		if(fft_idx == ARRAYSIZE_IN)
		{
			kiss_fftr(state, data, fft);
			draw(mode);

			fft_idx = 0;
		};

		int button = rb->button_get(false);
		switch(button)
		{
			case FFT_QUIT: run = false; break;

			/* rotate through all the modes */
			case FFT_MODE: mode += 1; if(mode > MODES) mode = 1; draw(mode); break;
		}
	}
    return PLUGIN_OK;
}
