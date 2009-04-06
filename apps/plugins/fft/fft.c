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
#include "lib/xlcd.h"
#include "math.h"

PLUGIN_HEADER

#if CONFIG_KEYPAD == RECORDER_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_F3
#   define FFT_WINDOW     BUTTON_F1
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_F3
#   define FFT_WINDOW     BUTTON_F1
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  (BUTTON_MENU | BUTTON_LEFT)
#   define FFT_WINDOW     (BUTTON_MENU | BUTTON_RIGHT)
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_REC
#   define FFT_WINDOW     BUTTON_SELECT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#   define MINESWP_SCROLLWHEEL
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH      BUTTON_RIGHT
#   define FFT_ORIENTATION  (BUTTON_SELECT | BUTTON_LEFT)
#   define FFT_WINDOW     (BUTTON_SELECT | BUTTON_RIGHT)
#   define FFT_SCALE         BUTTON_MENU
#   define FFT_COLOR       BUTTON_PLAY
#   define FFT_QUIT       (BUTTON_SELECT | BUTTON_MENU)

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW     BUTTON_PLAY
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW        BUTTON_A
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_WINDOW        BUTTON_REC
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  (BUTTON_SELECT | BUTTON_LEFT)
#   define FFT_WINDOW     (BUTTON_SELECT | BUTTON_RIGHT)
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_C200_PAD) || \
(CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
(CONFIG_KEYPAD == SANSA_M200_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH      BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_UP
#   define FFT_WINDOW        BUTTON_REC
#   define FFT_SCALE         BUTTON_SELECT
#   define FFT_COLOR       BUTTON_DOWN
#   define FFT_QUIT       BUTTON_POWER

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_FF
#   define FFT_WINDOW       BUTTON_SCROLL_UP
#   define FFT_SCALE            BUTTON_REW
#   define FFT_COLOR            BUTTON_PLAY
#   define FFT_QUIT             BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_MENU
#   define FFT_WINDOW     BUTTON_PREV
#   define FFT_SCALE            BUTTON_UP
#   define FFT_COLOR            BUTTON_DOWN
#   define FFT_QUIT             BUTTON_BACK

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_PLAY
#   define FFT_WINDOW     BUTTON_SELECT
#   define FFT_SCALE            BUTTON_UP
#   define FFT_COLOR            BUTTON_DOWN
#   define FFT_QUIT             BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#   define FFT_PREV_GRAPH       BUTTON_RC_REW
#   define FFT_NEXT_GRAPH       BUTTON_RC_FF
#   define FFT_ORIENTATION      BUTTON_RC_MODE
#   define FFT_WINDOW        BUTTON_RC_PLAY
#   define FFT_SCALE            BUTTON_RC_VOL_UP
#   define FFT_COLOR            BUTTON_RC_VOL_DOWN
#   define FFT_QUIT             BUTTON_RC_REC

#elif (CONFIG_KEYPAD == COWOND2_PAD)
#   define FFT_QUIT             BUTTON_POWER
#   define FFT_PREV_GRAPH       BUTTON_PLUS
#   define FFT_NEXT_GRAPH       BUTTON_MINUS

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_MENU
#   define FFT_WINDOW     BUTTON_SELECT
#   define FFT_SCALE            BUTTON_UP
#   define FFT_COLOR            BUTTON_DOWN
#   define FFT_QUIT             BUTTON_BACK

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_SELECT
#   define FFT_WINDOW     BUTTON_MENU
#   define FFT_SCALE            BUTTON_UP
#   define FFT_COLOR            BUTTON_DOWN
#   define FFT_QUIT             BUTTON_POWER

#else
#error No keymap defined!
#endif

#define FFT_SIZE (1024)

#if LCD_DEPTH > 1
#   ifdef HAVE_LCD_COLOR
#       define FG_GREEN LCD_RGBPACK(128, 255, 0)
#   endif
#endif

#include "kiss_fft.h"
#include "_kiss_fft_guts.h" /* sizeof(struct kiss_fft_state) */

#define ARRAYSIZE_IN FFT_SIZE
#define ARRAYSIZE_OUT FFT_SIZE
#define ARRAYSIZE_PLOT FFT_SIZE/2
#define BUFSIZE (sizeof(struct kiss_fft_state)+sizeof(kiss_fft_cpx)*(FFT_SIZE-1))
#define FFT_ALLOC kiss_fft_alloc
#define FFT_FFT   kiss_fft
#define FFT_CFG   kiss_fft_cfg

static kiss_fft_cpx input[ARRAYSIZE_IN];
static kiss_fft_cpx output[ARRAYSIZE_OUT];
static int32_t plot[ARRAYSIZE_PLOT];
static char buffer[BUFSIZE];

/************************* Math functions *************************/

#define QE float_q(2.718281828, 16)
#define LOGE_TEN 2.302585093
#define QLOGE_TEN float_q(LOGE_TEN, 16)

/* Returns logarithmically scaled values in S15.16 format */
int32_t get_log_value(int32_t value, bool fraction)
{
    int32_t result;

    /* ln (value)*/
    result = flog((fraction ? value : value << 16));

    /* Not worth it to add routines for negative values
     * (less than 2.6% of all values) */
    return (result < 0) ? 0 : result;
}

/* Apply window function to input
 * 0 - Hamming window
 * 1 - Hann window */
#define WINDOW_COUNT 2
void apply_window_func(char mode)
{
    switch(mode)
    {
        case 0: /* Hamming window */
        {
            const int32_t hamming_a = float_q(0.53836, 15), hamming_b =
                    float_q(0.46164, 15);
            size_t i;
            for (i = 0; i < ARRAYSIZE_IN; ++i)
            {
                int32_t cos;
                (void) fsincos(Q_DIV(i << 16, (ARRAYSIZE_IN - 1) << 16, 16) << 16,
                               &cos);
                cos >>= 16;

                /* value = value * (hamming_a - hamming_b * cos( 2 * pi * i/(ArraySize - 1) ) ) */
                input[i].r = Q15_MUL(input[i].r << 15,
                                   (hamming_a - Q15_MUL(cos, hamming_b))) >> 15;
                input[i].i = Q15_MUL(input[i].i << 15,
                                   (hamming_a - Q15_MUL(cos, hamming_b))) >> 15;
            }
            break;
        }
        case 1: /* Hann window */
        {
            size_t i;
            for (i = 0; i < ARRAYSIZE_IN; ++i)
            {
                int32_t factor;
                (void) fsincos(Q_DIV(i << 16, (ARRAYSIZE_IN - 1) << 16, 16) << 16,
                               &factor);
                /* s16.15; cos( 2* pi * i/(ArraySize - 1))*/
                factor >>= 16;
                /* 0.5 * cos( 2* pi * i/(ArraySize - 1))*/
                factor = Q15_MUL( (1 << 14), factor);
                /* 0.5 - 0.5 * cos( 2* pi * i/(ArraySize - 1)))*/
                factor = (1 << 14) - factor;

                input[i].r = Q15_MUL(input[i].r << 15, factor) >> 15;
                input[i].i = Q15_MUL(input[i].i << 15, factor) >> 15;
            }
            break;
        }
    }
}

/* Calculates the magnitudes from complex numbers and returns the maximum */
int32_t calc_magnitudes(bool logarithmic)
{
    int64_t tmp;
    size_t i;

    int32_t max = -2147483647;

    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        tmp = Q_MUL( ((int32_t) output[i].r) << 16,
                     ((int32_t) output[i].r) << 16, 16);
        tmp += Q_MUL( ((int32_t) output[i].i) << 16,
                      ((int32_t) output[i].i) << 16, 16);

        tmp = fsqrt(tmp & 0x7FFFFFFF , 16);

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

const unsigned char* modes_text[] = { "Lines", "Bars", "Spectrogram" };
const unsigned char* scales_text[] = { "Linear scale", "Logarithmic scale" };
const unsigned char* window_text[] = { "Hamming window", "Hann window" };
#define MODES_COUNT 3
#define REFRESH_RATE 20

struct {
    bool logarithmic;
    bool orientation_vertical;
    int window_func;
#ifdef HAVE_LCD_COLOR
    bool colored;
#endif
} graph_settings;
struct {
    int column;
    int row;
} spectrogram_settings;

#ifdef HAVE_LCD_COLOR
#define COLORS 256
static unsigned int colors[COLORS] = { LCD_RGBPACK(0, 0, 23), LCD_RGBPACK(0, 0, 26),
        LCD_RGBPACK(0, 0, 28), LCD_RGBPACK(0, 0, 31), LCD_RGBPACK(0, 0, 33),
        LCD_RGBPACK(0, 0, 36), LCD_RGBPACK(0, 0, 38), LCD_RGBPACK(0, 0, 41),
        LCD_RGBPACK(0, 0, 43), LCD_RGBPACK(0, 0, 46), LCD_RGBPACK(0, 0, 48),
        LCD_RGBPACK(0, 0, 50), LCD_RGBPACK(0, 0, 53), LCD_RGBPACK(0, 0, 55),
        LCD_RGBPACK(0, 0, 58), LCD_RGBPACK(0, 0, 59), LCD_RGBPACK(0, 0, 62),
        LCD_RGBPACK(0, 0, 64), LCD_RGBPACK(0, 0, 66), LCD_RGBPACK(0, 0, 69),
        LCD_RGBPACK(0, 0, 71), LCD_RGBPACK(0, 0, 73), LCD_RGBPACK(0, 0, 75),
        LCD_RGBPACK(0, 0, 77), LCD_RGBPACK(0, 0, 79), LCD_RGBPACK(1, 0, 81),
        LCD_RGBPACK(4, 0, 83), LCD_RGBPACK(7, 0, 85), LCD_RGBPACK(9, 0, 87),
        LCD_RGBPACK(9, 0, 87), LCD_RGBPACK(12, 0, 89), LCD_RGBPACK(15, 0, 91),
        LCD_RGBPACK(16, 0, 92), LCD_RGBPACK(19, 0, 94), LCD_RGBPACK(22, 0, 96),
        LCD_RGBPACK(24, 0, 97), LCD_RGBPACK(27, 0, 99),
        LCD_RGBPACK(30, 0, 101), LCD_RGBPACK(32, 0, 102),
        LCD_RGBPACK(35, 0, 104), LCD_RGBPACK(38, 0, 105),
        LCD_RGBPACK(40, 0, 107), LCD_RGBPACK(43, 0, 108),
        LCD_RGBPACK(45, 0, 109), LCD_RGBPACK(47, 0, 111),
        LCD_RGBPACK(50, 0, 112), LCD_RGBPACK(53, 0, 113),
        LCD_RGBPACK(55, 0, 114), LCD_RGBPACK(58, 0, 115),
        LCD_RGBPACK(60, 0, 116), LCD_RGBPACK(63, 0, 118),
        LCD_RGBPACK(65, 0, 119), LCD_RGBPACK(68, 0, 120),
        LCD_RGBPACK(70, 0, 120), LCD_RGBPACK(72, 0, 121),
        LCD_RGBPACK(75, 0, 122), LCD_RGBPACK(78, 0, 123),
        LCD_RGBPACK(80, 0, 123), LCD_RGBPACK(83, 0, 124),
        LCD_RGBPACK(85, 0, 125), LCD_RGBPACK(88, 0, 125),
        LCD_RGBPACK(90, 0, 126), LCD_RGBPACK(93, 0, 125),
        LCD_RGBPACK(95, 0, 126), LCD_RGBPACK(97, 0, 126),
        LCD_RGBPACK(99, 0, 127), LCD_RGBPACK(101, 0, 127),
        LCD_RGBPACK(101, 0, 127), LCD_RGBPACK(104, 0, 127),
        LCD_RGBPACK(106, 0, 126), LCD_RGBPACK(109, 0, 127),
        LCD_RGBPACK(111, 0, 127), LCD_RGBPACK(114, 0, 126),
        LCD_RGBPACK(116, 0, 127), LCD_RGBPACK(118, 0, 127),
        LCD_RGBPACK(120, 0, 127), LCD_RGBPACK(123, 0, 126),
        LCD_RGBPACK(125, 0, 125), LCD_RGBPACK(127, 0, 126),
        LCD_RGBPACK(130, 0, 125), LCD_RGBPACK(131, 0, 124),
        LCD_RGBPACK(134, 0, 124), LCD_RGBPACK(136, 0, 123),
        LCD_RGBPACK(138, 0, 122), LCD_RGBPACK(140, 0, 122),
        LCD_RGBPACK(142, 0, 121), LCD_RGBPACK(144, 0, 120),
        LCD_RGBPACK(147, 0, 119), LCD_RGBPACK(149, 0, 119),
        LCD_RGBPACK(151, 0, 118), LCD_RGBPACK(153, 0, 117),
        LCD_RGBPACK(155, 0, 115), LCD_RGBPACK(157, 0, 114),
        LCD_RGBPACK(159, 0, 113), LCD_RGBPACK(162, 0, 112),
        LCD_RGBPACK(164, 0, 111), LCD_RGBPACK(165, 0, 110),
        LCD_RGBPACK(167, 0, 109), LCD_RGBPACK(169, 0, 108),
        LCD_RGBPACK(171, 0, 106), LCD_RGBPACK(173, 0, 105),
        LCD_RGBPACK(175, 0, 103), LCD_RGBPACK(176, 0, 102),
        LCD_RGBPACK(178, 0, 100), LCD_RGBPACK(178, 0, 100),
        LCD_RGBPACK(180, 0, 98), LCD_RGBPACK(182, 0, 97),
        LCD_RGBPACK(184, 0, 95), LCD_RGBPACK(186, 0, 93),
        LCD_RGBPACK(188, 0, 91), LCD_RGBPACK(189, 0, 89),
        LCD_RGBPACK(190, 0, 87), LCD_RGBPACK(192, 0, 86),
        LCD_RGBPACK(194, 0, 84), LCD_RGBPACK(196, 0, 82),
        LCD_RGBPACK(197, 0, 80), LCD_RGBPACK(199, 0, 78),
        LCD_RGBPACK(201, 0, 76), LCD_RGBPACK(202, 0, 74),
        LCD_RGBPACK(204, 0, 72), LCD_RGBPACK(206, 0, 70),
        LCD_RGBPACK(207, 0, 67), LCD_RGBPACK(209, 0, 65),
        LCD_RGBPACK(210, 0, 63), LCD_RGBPACK(212, 0, 60),
        LCD_RGBPACK(213, 0, 58), LCD_RGBPACK(215, 0, 56),
        LCD_RGBPACK(216, 0, 53), LCD_RGBPACK(216, 0, 52),
        LCD_RGBPACK(218, 0, 49), LCD_RGBPACK(219, 0, 47),
        LCD_RGBPACK(221, 0, 44), LCD_RGBPACK(222, 0, 42),
        LCD_RGBPACK(223, 0, 39), LCD_RGBPACK(224, 0, 37),
        LCD_RGBPACK(226, 0, 34), LCD_RGBPACK(227, 0, 32),
        LCD_RGBPACK(228, 0, 29), LCD_RGBPACK(229, 0, 27),
        LCD_RGBPACK(230, 0, 25), LCD_RGBPACK(231, 0, 22),
        LCD_RGBPACK(232, 0, 19), LCD_RGBPACK(232, 0, 19),
        LCD_RGBPACK(233, 0, 17), LCD_RGBPACK(235, 0, 14),
        LCD_RGBPACK(236, 0, 12), LCD_RGBPACK(237, 0, 9),
        LCD_RGBPACK(238, 0, 6), LCD_RGBPACK(239, 0, 4), LCD_RGBPACK(239, 0, 1),
        LCD_RGBPACK(240, 1, 0), LCD_RGBPACK(241, 6, 0),
        LCD_RGBPACK(242, 11, 0), LCD_RGBPACK(243, 16, 0),
        LCD_RGBPACK(244, 21, 0), LCD_RGBPACK(244, 26, 0),
        LCD_RGBPACK(245, 31, 0), LCD_RGBPACK(246, 36, 0),
        LCD_RGBPACK(247, 41, 0), LCD_RGBPACK(247, 46, 0),
        LCD_RGBPACK(247, 50, 0), LCD_RGBPACK(248, 55, 0),
        LCD_RGBPACK(248, 60, 0), LCD_RGBPACK(249, 65, 0),
        LCD_RGBPACK(249, 70, 0), LCD_RGBPACK(250, 75, 0),
        LCD_RGBPACK(250, 79, 0), LCD_RGBPACK(251, 84, 0),
        LCD_RGBPACK(252, 89, 0), LCD_RGBPACK(251, 94, 0),
        LCD_RGBPACK(252, 99, 0), LCD_RGBPACK(253, 103, 0),
        LCD_RGBPACK(252, 108, 0),
        LCD_RGBPACK(253, 112, 0), LCD_RGBPACK(254, 117, 0),
        LCD_RGBPACK(254, 121, 0), LCD_RGBPACK(253, 125, 0),
        LCD_RGBPACK(254, 130, 0), LCD_RGBPACK(255, 134, 0),
        LCD_RGBPACK(255, 137, 0), LCD_RGBPACK(255, 138, 0),
        LCD_RGBPACK(255, 142, 0), LCD_RGBPACK(255, 147, 0),
        LCD_RGBPACK(255, 151, 0), LCD_RGBPACK(255, 154, 0),
        LCD_RGBPACK(255, 158, 0), LCD_RGBPACK(255, 162, 0),
        LCD_RGBPACK(255, 166, 0), LCD_RGBPACK(255, 170, 0),
        LCD_RGBPACK(255, 174, 0), LCD_RGBPACK(255, 178, 0),
        LCD_RGBPACK(255, 181, 0), LCD_RGBPACK(255, 184, 0),
        LCD_RGBPACK(255, 187, 0), LCD_RGBPACK(255, 191, 0),
        LCD_RGBPACK(255, 194, 0), LCD_RGBPACK(255, 197, 0),
        LCD_RGBPACK(255, 201, 0), LCD_RGBPACK(255, 204, 3),
        LCD_RGBPACK(255, 207, 8), LCD_RGBPACK(255, 210, 12),
        LCD_RGBPACK(255, 213, 17), LCD_RGBPACK(255, 215, 21),
        LCD_RGBPACK(255, 217, 26), LCD_RGBPACK(255, 220,30),
        LCD_RGBPACK(255, 223, 34), LCD_RGBPACK(255, 225, 39),
        LCD_RGBPACK(255, 228, 44),
        LCD_RGBPACK(255, 229, 48), LCD_RGBPACK(255, 231, 53),
        LCD_RGBPACK(255, 233, 57),
        LCD_RGBPACK(255, 235, 61), LCD_RGBPACK(255, 237, 66),
        LCD_RGBPACK(255, 239, 71),
        LCD_RGBPACK(255, 241, 75), LCD_RGBPACK(255, 243, 80),
        LCD_RGBPACK(255, 244, 84),
        LCD_RGBPACK(255, 246, 88), LCD_RGBPACK(255, 246, 88),
        LCD_RGBPACK(255, 247, 93),
        LCD_RGBPACK(255, 248, 97), LCD_RGBPACK(255, 249, 102),
        LCD_RGBPACK(255, 250, 107), LCD_RGBPACK(255, 251, 111),
        LCD_RGBPACK(255, 251, 115), LCD_RGBPACK(255, 252, 120),
        LCD_RGBPACK(255, 252, 124), LCD_RGBPACK(255, 253, 129),
        LCD_RGBPACK(255, 253, 133), LCD_RGBPACK(255, 254, 138),
        LCD_RGBPACK(255, 255, 143), LCD_RGBPACK(255, 255, 147),
        LCD_RGBPACK(255, 255, 151), LCD_RGBPACK(255, 255, 156),
        LCD_RGBPACK(255, 255, 160), LCD_RGBPACK(255, 255, 165),
        LCD_RGBPACK(255, 255, 170), LCD_RGBPACK(255, 255, 174),
        LCD_RGBPACK(255, 255, 178), LCD_RGBPACK(255, 255, 183),
        LCD_RGBPACK(255, 255, 187), LCD_RGBPACK(255, 255, 192),
        LCD_RGBPACK(255, 255, 196), LCD_RGBPACK(255, 255, 200),
        LCD_RGBPACK(255, 255, 205), LCD_RGBPACK(255, 255, 210),
        LCD_RGBPACK(255, 255, 214), LCD_RGBPACK(255, 255, 219),
        LCD_RGBPACK(255, 255, 223), LCD_RGBPACK(255, 255, 227),
        LCD_RGBPACK(255, 255, 232), LCD_RGBPACK(255, 255, 236),
        LCD_RGBPACK(255, 255, 241), LCD_RGBPACK(255, 255, 246),
        LCD_RGBPACK(255, 255, 250), LCD_RGBPACK(255, 255, 255) };
#endif
static long next_update = 0;

void draw_lines_vertical(void);
void draw_lines_horizontal(void);
void draw_bars_vertical(void);
void draw_bars_horizontal(void);
void draw_spectrogram_vertical(void);
void draw_spectrogram_horizontal(void);

void draw(char mode, const unsigned char* message)
{
    static uint32_t show_message = 0, last_mode = 0;
    static bool last_orientation = true;
    static unsigned char* last_message = 0;
    if (message != 0)
    {
        last_message = (unsigned char*) message;
        show_message = 5;
    }

    switch (mode)
    {
        default:
        case 0: {
            rb->lcd_clear_display();

            if (graph_settings.orientation_vertical)
                draw_lines_vertical();
            else
                draw_lines_horizontal();

            last_mode = 0;
            spectrogram_settings.row = 0; spectrogram_settings.column = 0;
            break;
        }
        case 1: {
            rb->lcd_clear_display();

            if(graph_settings.orientation_vertical)
                draw_bars_vertical();
            else
                draw_bars_horizontal();

            last_mode = 1;
            spectrogram_settings.row = 0; spectrogram_settings.column = 0;
            break;
        }
        case 2: {
            if(last_mode != 2 ||
               graph_settings.orientation_vertical != last_orientation)
            {
                spectrogram_settings.row = 0; spectrogram_settings.column = 0;
                rb->lcd_clear_display();
            }
            if(graph_settings.orientation_vertical)
                draw_spectrogram_vertical();
            else
                draw_spectrogram_horizontal();

            last_mode = 2;
            break;
        }
    }

    if (show_message > 0)
    {
        int x, y;
        rb->lcd_getstringsize(last_message, &x, &y);
        x += 6; /* 3 px of horizontal padding and */
        y += 4; /* 2 px of vertical padding */

        if(mode == 2)
        {
            if (graph_settings.orientation_vertical)
            {
                if(spectrogram_settings.column > LCD_WIDTH-x-2)
                {
                    xlcd_scroll_left(spectrogram_settings.column -
                                     (LCD_WIDTH - x - 1));
                    spectrogram_settings.column = LCD_WIDTH - x - 2;
                }
            }
        }

        rb->lcd_set_foreground(LCD_DARKGRAY);
        rb->lcd_fillrect(LCD_WIDTH-1-x, 0, LCD_WIDTH-1, y);

        rb->lcd_set_foreground(LCD_DEFAULT_FG);
        rb->lcd_set_background(LCD_DARKGRAY);
        rb->lcd_putsxy(LCD_WIDTH-1-x+3, 2, last_message);
        rb->lcd_set_background(LCD_DEFAULT_BG);

        show_message--;
        if(show_message == 0 && mode == 2)
        {
            if(graph_settings.orientation_vertical)
            {
                rb->lcd_set_drawmode(DRMODE_SOLID | DRMODE_INVERSEVID);
                rb->lcd_fillrect(LCD_WIDTH-1-x, 0, LCD_WIDTH-1, y);
                rb->lcd_set_drawmode(DRMODE_SOLID);
            }
            else
            {
                xlcd_scroll_up(y);
                spectrogram_settings.row -= y;
                if(spectrogram_settings.row < 0)
                    spectrogram_settings.row = 0;
            }
        }
    }

    last_orientation = graph_settings.orientation_vertical;

    rb->lcd_update();

    /* we still have time in our time slot, so we sleep() */
    if (*rb->current_tick < next_update)
        rb->sleep(next_update - *rb->current_tick);

    /* end of next time slot */
    next_update = *rb->current_tick + HZ / REFRESH_RATE;
}

void draw_lines_vertical(void)
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

#ifdef HAVE_LCD_COLOR
    if(graph_settings.colored)
    {
        int line;
        for(line = 0; line < LCD_HEIGHT; ++line)
        {
            int32_t color = Q_DIV((line+1) << 16, LCD_HEIGHT << 16, 16);
            color = Q_MUL(color, (COLORS-1) << 16, 16) >> 16;
            rb->lcd_set_foreground(colors[color]);
            rb->lcd_drawline(0, LCD_HEIGHT-line-1,
                             LCD_WIDTH-1, LCD_HEIGHT-line-1);
        }
        /* Erase the lines with the background color */
        rb->lcd_set_foreground(rb->lcd_get_background());
    }
#endif

    /* take the average of neighboring bins
     * if we have to scale the graph horizontally */
    int64_t bins_avg = 0;
    bool draw = true;
    int32_t i;
    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        int32_t x = 0, y = 0;

        x = Q15_MUL(hfactor, i << 15);
        x += (1 << 14);
        x >>= 15;

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
        {
#       ifdef HAVE_LCD_COLOR
            if(graph_settings.colored)
                rb->lcd_drawline(x, 0, x, LCD_HEIGHT-y);
            else
#       endif
            rb->lcd_drawline(x, LCD_HEIGHT-1 , x, LCD_HEIGHT-y-1);
        }
    }
#   ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_DEFAULT_FG);
#   endif
}

void draw_lines_horizontal(void)
{
    static int max = 0;
    static bool last_mode = false;

    static const int32_t vfactor =
            Q15_DIV(LCD_HEIGHT << 15, (ARRAYSIZE_PLOT) << 15),
            bins_per_pixel = (ARRAYSIZE_PLOT) / LCD_HEIGHT;

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

    int32_t hfactor;

    if (graph_settings.logarithmic)
        hfactor = Q_DIV((LCD_WIDTH - 1) << 16, max, 16); /* s15.16 */
    else
        hfactor = Q15_DIV((LCD_WIDTH - 1) << 15, max << 15); /* s16.15 */

#ifdef HAVE_LCD_COLOR
    if(graph_settings.colored)
    {
        int line;
        for(line = 0; line < LCD_WIDTH; ++line)
        {
            int32_t color = Q_DIV((line+1) << 16, LCD_WIDTH << 16, 16);
            color = Q_MUL(color, (COLORS-1) << 16, 16) >> 16;
            rb->lcd_set_foreground(colors[color]);
            rb->lcd_drawline(line, 0, line, LCD_HEIGHT-1);
        }
        /* Erase the lines with the background color */
        rb->lcd_set_foreground(rb->lcd_get_background());
    }
#endif

    /* take the average of neighboring bins
     * if we have to scale the graph horizontally */
    int64_t bins_avg = 0;
    bool draw = true;
    int32_t i;
    for (i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        int32_t x = 0, y = 0;

        y = Q15_MUL(vfactor, i << 15) + (1 << 14);
        y >>= 15;

        if (vfactor < 32768) /* vfactor < 0, graph compression */
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

                    x = Q_MUL(hfactor, bins_avg, 16) >> 16;
                }
                else
                {
                    bins_avg = Q15_DIV(bins_avg << 15, div << 15);
                    bins_avg += (1 << 14); /* rounding up */

                    x = Q15_MUL(hfactor, bins_avg) >> 15;
                }

                bins_avg = 0;
                draw = true;
            }
        }
        else
        {
            y = Q15_MUL(hfactor, plot[i] << 15) >> 15;
            draw = true;
        }

        if (draw)
        {
#       ifdef HAVE_LCD_COLOR
            if(graph_settings.colored)
            {
                rb->lcd_drawline(LCD_WIDTH-1, y, x, y);
            }
            else
#       endif
            rb->lcd_drawline(0, y, x, y);
        }
    }
#   ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_DEFAULT_FG);
#   endif
}

void draw_bars_vertical(void)
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

void draw_bars_horizontal(void)
{
    static const unsigned int bars = 12, border = 3, items = ARRAYSIZE_PLOT
            / bars, height = (LCD_HEIGHT - ((bars - 1) * border)) / bars;

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

    int64_t hfactor;
    if (graph_settings.logarithmic)
        hfactor = Q_DIV(LCD_WIDTH << 16, bars_max, 16);
    else
        hfactor = Q15_DIV(LCD_WIDTH << 15, bars_max);

    for (i = 0; i < bars; ++i)
    {
        int y = (i) * (border + height);
        int x;
        if (graph_settings.logarithmic)
        {
            x = Q_MUL(hfactor, bars_values[i], 16);
            x += (1 << 15);
            x >>= 16;
        }
        else
        {
            x = Q15_MUL(hfactor, bars_values[i]);
            x += (1 << 14);
            x >>= 15;
        }

        rb->lcd_fillrect(0, y, x, height);
    }
}

#define QDB_MAX 454934
#define LIN_MAX 1034
void draw_spectrogram_vertical(void)
{
    const int32_t scale_factor =
            ( Q15_DIV(ARRAYSIZE_PLOT << 15, LCD_HEIGHT << 15) + (1<<14) ) >> 15,
        remaining_div =
            ( Q15_DIV((scale_factor*LCD_HEIGHT) << 15,
                      (ARRAYSIZE_PLOT-scale_factor*LCD_HEIGHT) << 15)
             + (1<<14) ) >> 15;

    static int max = 0;
    int nmax = calc_magnitudes(graph_settings.logarithmic);
    if (nmax > max)
        max = nmax, DEBUGF("New maximum: %i\n", max);

    int i, y = LCD_HEIGHT-1, count = 0, rem_count = 0;
    int64_t avg = 0;
    for(i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        avg += plot[i];
        ++count;
        ++rem_count;

        /* Kinda hacky - due to the rounding in scale_factor, we try to
         * uniformly interweave the extra values in our calculations */
        if(remaining_div > 0 && rem_count == remaining_div &&
                (i+1) < ARRAYSIZE_PLOT)
        {
            ++i;
            avg += plot[i];
            rem_count = 0;
        }

        if(count >= scale_factor)
        {
            if(rem_count == 0) /* if we just added an extra value */
                ++count;

            int32_t color;
            if(graph_settings.logarithmic)
            {
                avg = Q_DIV(avg, count << 16, 16);
                color = Q_DIV(avg, QDB_MAX, 16);
                color = Q_MUL(color, (COLORS-1) << 16, 16) >> 16;
            }
            else
            {
                avg = Q15_DIV(avg << 15, count << 15);
                color = Q15_DIV(avg, LIN_MAX << 15);
                color = Q15_MUL(color, (COLORS-1) << 15) >> 15;
            }
            rb->lcd_set_foreground(colors[color]);
            rb->lcd_drawpixel(spectrogram_settings.column, y);

            y--;

            avg = 0;
            count = 0;
        }
    }
    if(spectrogram_settings.column != LCD_WIDTH - 1)
        spectrogram_settings.column++;
    else
        xlcd_scroll_left(1);
}
void draw_spectrogram_horizontal(void)
{
    const int32_t scale_factor =
            ( Q15_DIV(ARRAYSIZE_PLOT << 15, LCD_WIDTH << 15)) >> 15,
        remaining_div =
            ( Q15_DIV((scale_factor*LCD_WIDTH) << 15,
                      (ARRAYSIZE_PLOT-scale_factor*LCD_WIDTH) << 15)
             + (1<<14) ) >> 15;

    calc_magnitudes(graph_settings.logarithmic);

    int i, x = 0, count = 0, rem_count = 0;
    int64_t avg = 0;
    for(i = 0; i < ARRAYSIZE_PLOT; ++i)
    {
        avg += plot[i];
        ++count;
        ++rem_count;

        /* Kinda hacky - due to the rounding in scale_factor, we try to
         * uniformly interweave the extra values in our calculations */
        if(remaining_div > 0 && rem_count == remaining_div &&
                (i+1) < ARRAYSIZE_PLOT)
        {
            ++i;
            avg += plot[i];
            rem_count = 0;
        }

        if(count >= scale_factor)
        {
            if(rem_count == 0) /* if we just added an extra value */
                ++count;

            int32_t color;
            if(graph_settings.logarithmic)
            {
                avg = Q_DIV(avg, count << 16, 16);
                color = Q_DIV(avg, QDB_MAX, 16);
                color = Q_MUL(color, (COLORS-1) << 16, 16) >> 16;
            }
            else
            {
                avg = Q15_DIV(avg << 15, count << 15);
                color = Q15_DIV(avg, LIN_MAX << 15);
                color = Q15_MUL(color, (COLORS-1) << 15) >> 15;
            }
            rb->lcd_set_foreground(colors[color]);
            rb->lcd_drawpixel(x, spectrogram_settings.row);

            x++;

            avg = 0;
            count = 0;
        }
    }
    if(spectrogram_settings.row != LCD_HEIGHT-1)
        spectrogram_settings.row++;
    else
        xlcd_scroll_up(1);
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

    /* Defaults */
    bool run = true;
    int mode = 0;
    graph_settings.logarithmic = true;
    graph_settings.orientation_vertical = true;
    graph_settings.window_func = 0;
#ifdef HAVE_LCD_COLOR
    graph_settings.colored = false; /* doesn't do anything yet*/
#endif
    bool changed_window = false;

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

            input[fft_idx].r = left;
            input[fft_idx].i = right;
            fft_idx++;

            if (fft_idx == ARRAYSIZE_IN)
                break;
        } while (idx < count);

        if (fft_idx == ARRAYSIZE_IN)
        {
            apply_window_func(graph_settings.window_func);

            /* Play nice - the sleep at the end of draw()
             * only tries to maintain the frame rate */
            rb->yield();
            FFT_FFT(state, input, output);
            if(changed_window)
            {
                draw(mode, window_text[graph_settings.window_func]);
                changed_window = false;
            }
            else
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
            case FFT_WINDOW: {
                changed_window = true;
                graph_settings.window_func ++;
                if(graph_settings.window_func >= WINDOW_COUNT)
                    graph_settings.window_func = 0;
                break;
            }
            case FFT_SCALE: {
                graph_settings.logarithmic = !graph_settings.logarithmic;
                draw(mode, scales_text[graph_settings.logarithmic ? 1 : 0]);
                break;
            }
            case FFT_ORIENTATION: {
                graph_settings.orientation_vertical = !graph_settings.orientation_vertical;
                draw(mode, 0);
                break;
            }
#           ifdef HAVE_LCD_COLOR
            case FFT_COLOR: {
                graph_settings.colored = !graph_settings.colored;
                draw(mode, 0);
                break;
            }
#           endif
            default: {
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
            }

        }
    }
    backlight_use_settings();
    return PLUGIN_OK;
}
