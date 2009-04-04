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
#   define FFT_ORIENTATION  BUTTON_F3
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_F3
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  (BUTTON_MENU | BUTTON_LEFT)
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_REC
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
#   define FFT_SCALE         BUTTON_MENU
#   define FFT_COLOR       BUTTON_PLAY
#   define FFT_QUIT       (BUTTON_SELECT | BUTTON_MENU)

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)

#   define MINESWP_SCROLLWHEEL
#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_SELECT
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)

#   define FFT_PREV_GRAPH     BUTTON_LEFT
#   define FFT_NEXT_GRAPH    BUTTON_RIGHT
#   define FFT_ORIENTATION  (BUTTON_SELECT | BUTTON_LEFT)
#   define FFT_SCALE       BUTTON_UP
#   define FFT_COLOR     BUTTON_DOWN
#   define FFT_QUIT     BUTTON_POWER

#elif (CONFIG_KEYPAD == SANSA_C200_PAD) || \
(CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
(CONFIG_KEYPAD == SANSA_M200_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH      BUTTON_RIGHT
#   define FFT_ORIENTATION  BUTTON_UP
#   define FFT_SCALE         BUTTON_SELECT
#   define FFT_COLOR       BUTTON_DOWN
#   define FFT_QUIT       BUTTON_POWER

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_FF
#   define FFT_SCALE            BUTTON_REW
#   define FFT_COLOR            BUTTON_PLAY
#   define FFT_QUIT             BUTTON_POWER

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_MENU
#   define FFT_SCALE            BUTTON_UP
#   define FFT_COLOR            BUTTON_DOWN
#   define FFT_QUIT             BUTTON_BACK

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_PLAY
#   define FFT_SCALE            BUTTON_UP
#   define FFT_COLOR            BUTTON_DOWN
#   define FFT_QUIT             BUTTON_POWER

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#   define FFT_PREV_GRAPH       BUTTON_RC_REW
#   define FFT_NEXT_GRAPH       BUTTON_RC_FF
#   define FFT_ORIENTATION      BUTTON_RC_MODE
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
#   define FFT_SCALE            BUTTON_UP
#   define FFT_COLOR            BUTTON_DOWN
#   define FFT_QUIT             BUTTON_BACK

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#   define FFT_PREV_GRAPH       BUTTON_LEFT
#   define FFT_NEXT_GRAPH       BUTTON_RIGHT
#   define FFT_ORIENTATION      BUTTON_SELECT
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
    bool orientation_vertical;
#ifdef HAVE_LCD_COLOR
    bool colored;
#endif
} graph_settings;

static unsigned int
        colors[256] = { LCD_RGBPACK(0, 0, 0), LCD_RGBPACK(1, 1, 1), LCD_RGBPACK(2, 2, 2),
                LCD_RGBPACK(3, 3, 3), LCD_RGBPACK(4, 4, 4), LCD_RGBPACK(5, 5, 5),
                LCD_RGBPACK(6, 6, 6), LCD_RGBPACK(7, 7, 7), LCD_RGBPACK(8, 8, 8),
                LCD_RGBPACK(9, 9, 9), LCD_RGBPACK(10, 10, 10), LCD_RGBPACK(11, 11, 11),
                LCD_RGBPACK(12, 12, 12), LCD_RGBPACK(13, 13, 13), LCD_RGBPACK(14, 14, 14),
                LCD_RGBPACK(15, 15, 15), LCD_RGBPACK(16, 16, 16), LCD_RGBPACK(17, 17, 17),
                LCD_RGBPACK(18, 18, 18), LCD_RGBPACK(19, 19, 19), LCD_RGBPACK(20, 20, 20),
                LCD_RGBPACK(21, 21, 21), LCD_RGBPACK(22, 22, 22), LCD_RGBPACK(23, 23, 23),
                LCD_RGBPACK(24, 24, 24), LCD_RGBPACK(25, 25, 25), LCD_RGBPACK(26, 26, 26),
                LCD_RGBPACK(27, 27, 27), LCD_RGBPACK(28, 28, 28), LCD_RGBPACK(29, 29, 29),
                LCD_RGBPACK(30, 30, 30), LCD_RGBPACK(31, 31, 31), LCD_RGBPACK(32, 32, 32),
                LCD_RGBPACK(33, 33, 33), LCD_RGBPACK(34, 34, 34), LCD_RGBPACK(35, 35, 35),
                LCD_RGBPACK(36, 36, 36), LCD_RGBPACK(37, 37, 37), LCD_RGBPACK(38, 38, 38),
                LCD_RGBPACK(39, 39, 39), LCD_RGBPACK(40, 40, 40), LCD_RGBPACK(41, 41, 41),
                LCD_RGBPACK(42, 42, 42), LCD_RGBPACK(43, 43, 43), LCD_RGBPACK(44, 44, 44),
                LCD_RGBPACK(45, 45, 45), LCD_RGBPACK(46, 46, 46), LCD_RGBPACK(47, 47, 47),
                LCD_RGBPACK(48, 48, 48), LCD_RGBPACK(49, 49, 49), LCD_RGBPACK(50, 50, 50),
                LCD_RGBPACK(51, 51, 51), LCD_RGBPACK(52, 52, 52), LCD_RGBPACK(53, 53, 53),
                LCD_RGBPACK(54, 54, 54), LCD_RGBPACK(55, 55, 55), LCD_RGBPACK(56, 56, 56),
                LCD_RGBPACK(57, 57, 57), LCD_RGBPACK(58, 58, 58), LCD_RGBPACK(59, 59, 59),
                LCD_RGBPACK(60, 60, 60), LCD_RGBPACK(61, 61, 61), LCD_RGBPACK(62, 62, 62),
                LCD_RGBPACK(63, 63, 63), LCD_RGBPACK(64, 64, 64), LCD_RGBPACK(65, 65, 65),
                LCD_RGBPACK(66, 66, 66), LCD_RGBPACK(67, 67, 67), LCD_RGBPACK(68, 68, 68),
                LCD_RGBPACK(69, 69, 69), LCD_RGBPACK(70, 70, 70), LCD_RGBPACK(71, 71, 71),
                LCD_RGBPACK(72, 72, 72), LCD_RGBPACK(73, 73, 73), LCD_RGBPACK(74, 74, 74),
                LCD_RGBPACK(75, 75, 75), LCD_RGBPACK(76, 76, 76), LCD_RGBPACK(77, 77, 77),
                LCD_RGBPACK(78, 78, 78), LCD_RGBPACK(79, 79, 79), LCD_RGBPACK(80, 80, 80),
                LCD_RGBPACK(81, 81, 81), LCD_RGBPACK(82, 82, 82), LCD_RGBPACK(83, 83, 83),
                LCD_RGBPACK(84, 84, 84), LCD_RGBPACK(85, 85, 85), LCD_RGBPACK(86, 86, 86),
                LCD_RGBPACK(87, 87, 87), LCD_RGBPACK(88, 88, 88), LCD_RGBPACK(89, 89, 89),
                LCD_RGBPACK(90, 90, 90), LCD_RGBPACK(91, 91, 91), LCD_RGBPACK(92, 92, 92),
                LCD_RGBPACK(93, 93, 93), LCD_RGBPACK(94, 94, 94), LCD_RGBPACK(95, 95, 95),
                LCD_RGBPACK(96, 96, 96), LCD_RGBPACK(97, 97, 97), LCD_RGBPACK(98, 98, 98),
                LCD_RGBPACK(99, 99, 99), LCD_RGBPACK(100, 100, 100), LCD_RGBPACK(101, 101,
                                                                     101),
                LCD_RGBPACK(102, 102, 102), LCD_RGBPACK(103, 103, 103), LCD_RGBPACK(104,
                                                                        104,
                                                                        104),
                LCD_RGBPACK(105, 105, 105), LCD_RGBPACK(106, 106, 106), LCD_RGBPACK(107,
                                                                        107,
                                                                        107),
                LCD_RGBPACK(108, 108, 108), LCD_RGBPACK(109, 109, 109), LCD_RGBPACK(110,
                                                                        110,
                                                                        110),
                LCD_RGBPACK(111, 111, 111), LCD_RGBPACK(112, 112, 112), LCD_RGBPACK(113,
                                                                        113,
                                                                        113),
                LCD_RGBPACK(114, 114, 114), LCD_RGBPACK(115, 115, 115), LCD_RGBPACK(116,
                                                                        116,
                                                                        116),
                LCD_RGBPACK(117, 117, 117), LCD_RGBPACK(118, 118, 118), LCD_RGBPACK(119,
                                                                        119,
                                                                        119),
                LCD_RGBPACK(120, 120, 120), LCD_RGBPACK(121, 121, 121), LCD_RGBPACK(122,
                                                                        122,
                                                                        122),
                LCD_RGBPACK(123, 123, 123), LCD_RGBPACK(124, 124, 124), LCD_RGBPACK(125,
                                                                        125,
                                                                        125),
                LCD_RGBPACK(126, 126, 126), LCD_RGBPACK(127, 127, 127), LCD_RGBPACK(128,
                                                                        128,
                                                                        128),
                LCD_RGBPACK(129, 129, 129), LCD_RGBPACK(130, 130, 130), LCD_RGBPACK(131,
                                                                        131,
                                                                        131),
                LCD_RGBPACK(132, 132, 132), LCD_RGBPACK(133, 133, 133), LCD_RGBPACK(134,
                                                                        134,
                                                                        134),
                LCD_RGBPACK(135, 135, 135), LCD_RGBPACK(136, 136, 136), LCD_RGBPACK(137,
                                                                        137,
                                                                        137),
                LCD_RGBPACK(138, 138, 138), LCD_RGBPACK(139, 139, 139), LCD_RGBPACK(140,
                                                                        140,
                                                                        140),
                LCD_RGBPACK(141, 141, 141), LCD_RGBPACK(142, 142, 142), LCD_RGBPACK(143,
                                                                        143,
                                                                        143),
                LCD_RGBPACK(144, 144, 144), LCD_RGBPACK(145, 145, 145), LCD_RGBPACK(146,
                                                                        146,
                                                                        146),
                LCD_RGBPACK(147, 147, 147), LCD_RGBPACK(148, 148, 148), LCD_RGBPACK(149,
                                                                        149,
                                                                        149),
                LCD_RGBPACK(150, 150, 150), LCD_RGBPACK(151, 151, 151), LCD_RGBPACK(152,
                                                                        152,
                                                                        152),
                LCD_RGBPACK(153, 153, 153), LCD_RGBPACK(154, 154, 154), LCD_RGBPACK(155,
                                                                        155,
                                                                        155),
                LCD_RGBPACK(156, 156, 156), LCD_RGBPACK(157, 157, 157), LCD_RGBPACK(158,
                                                                        158,
                                                                        158),
                LCD_RGBPACK(159, 159, 159), LCD_RGBPACK(160, 160, 160), LCD_RGBPACK(161,
                                                                        161,
                                                                        161),
                LCD_RGBPACK(162, 162, 162), LCD_RGBPACK(163, 163, 163), LCD_RGBPACK(164,
                                                                        164,
                                                                        164),
                LCD_RGBPACK(165, 165, 165), LCD_RGBPACK(166, 166, 166), LCD_RGBPACK(167,
                                                                        167,
                                                                        167),
                LCD_RGBPACK(168, 168, 168), LCD_RGBPACK(169, 169, 169), LCD_RGBPACK(170,
                                                                        170,
                                                                        170),
                LCD_RGBPACK(171, 171, 171), LCD_RGBPACK(172, 172, 172), LCD_RGBPACK(173,
                                                                        173,
                                                                        173),
                LCD_RGBPACK(174, 174, 174), LCD_RGBPACK(175, 175, 175), LCD_RGBPACK(176,
                                                                        176,
                                                                        176),
                LCD_RGBPACK(177, 177, 177), LCD_RGBPACK(178, 178, 178), LCD_RGBPACK(179,
                                                                        179,
                                                                        179),
                LCD_RGBPACK(180, 180, 180), LCD_RGBPACK(181, 181, 181), LCD_RGBPACK(182,
                                                                        182,
                                                                        182),
                LCD_RGBPACK(183, 183, 183), LCD_RGBPACK(184, 184, 184), LCD_RGBPACK(185,
                                                                        185,
                                                                        185),
                LCD_RGBPACK(186, 186, 186), LCD_RGBPACK(187, 187, 187), LCD_RGBPACK(188,
                                                                        188,
                                                                        188),
                LCD_RGBPACK(189, 189, 189), LCD_RGBPACK(190, 190, 190), LCD_RGBPACK(191,
                                                                        191,
                                                                        191),
                LCD_RGBPACK(192, 192, 192), LCD_RGBPACK(193, 193, 193), LCD_RGBPACK(194,
                                                                        194,
                                                                        194),
                LCD_RGBPACK(195, 195, 195), LCD_RGBPACK(196, 196, 196), LCD_RGBPACK(197,
                                                                        197,
                                                                        197),
                LCD_RGBPACK(198, 198, 198), LCD_RGBPACK(199, 199, 199), LCD_RGBPACK(200,
                                                                        200,
                                                                        200),
                LCD_RGBPACK(201, 201, 201), LCD_RGBPACK(202, 202, 202), LCD_RGBPACK(203,
                                                                        203,
                                                                        203),
                LCD_RGBPACK(204, 204, 204), LCD_RGBPACK(205, 205, 205), LCD_RGBPACK(206,
                                                                        206,
                                                                        206),
                LCD_RGBPACK(207, 207, 207), LCD_RGBPACK(208, 208, 208), LCD_RGBPACK(209,
                                                                        209,
                                                                        209),
                LCD_RGBPACK(210, 210, 210), LCD_RGBPACK(211, 211, 211), LCD_RGBPACK(212,
                                                                        212,
                                                                        212),
                LCD_RGBPACK(213, 213, 213), LCD_RGBPACK(214, 214, 214), LCD_RGBPACK(215,
                                                                        215,
                                                                        215),
                LCD_RGBPACK(216, 216, 216), LCD_RGBPACK(217, 217, 217), LCD_RGBPACK(218,
                                                                        218,
                                                                        218),
                LCD_RGBPACK(219, 219, 219), LCD_RGBPACK(220, 220, 220), LCD_RGBPACK(221,
                                                                        221,
                                                                        221),
                LCD_RGBPACK(222, 222, 222), LCD_RGBPACK(223, 223, 223), LCD_RGBPACK(224,
                                                                        224,
                                                                        224),
                LCD_RGBPACK(225, 225, 225), LCD_RGBPACK(226, 226, 226), LCD_RGBPACK(227,
                                                                        227,
                                                                        227),
                LCD_RGBPACK(228, 228, 228), LCD_RGBPACK(229, 229, 229), LCD_RGBPACK(230,
                                                                        230,
                                                                        230),
                LCD_RGBPACK(231, 231, 231), LCD_RGBPACK(232, 232, 232), LCD_RGBPACK(233,
                                                                        233,
                                                                        233),
                LCD_RGBPACK(234, 234, 234), LCD_RGBPACK(235, 235, 235), LCD_RGBPACK(236,
                                                                        236,
                                                                        236),
                LCD_RGBPACK(237, 237, 237), LCD_RGBPACK(238, 238, 238), LCD_RGBPACK(239,
                                                                        239,
                                                                        239),
                LCD_RGBPACK(240, 240, 240), LCD_RGBPACK(241, 241, 241), LCD_RGBPACK(242,
                                                                        242,
                                                                        242),
                LCD_RGBPACK(243, 243, 243), LCD_RGBPACK(244, 244, 244), LCD_RGBPACK(245,
                                                                        245,
                                                                        245),
                LCD_RGBPACK(246, 246, 246), LCD_RGBPACK(247, 247, 247), LCD_RGBPACK(248,
                                                                        248,
                                                                        248),
                LCD_RGBPACK(249, 249, 249), LCD_RGBPACK(250, 250, 250), LCD_RGBPACK(251,
                                                                        251,
                                                                        251),
                LCD_RGBPACK(252, 252, 252), LCD_RGBPACK(253, 253, 253), LCD_RGBPACK(254,
                                                                        254,
                                                                        254),
                LCD_RGBPACK(255, 255, 255) };

static long next_update = 0;

void draw_lines_vertical(void);
void draw_lines_horizontal(void);
void draw_bars_vertical(void);
void draw_bars_horizontal(void);

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
            if (graph_settings.orientation_vertical)
                draw_lines_vertical();
            else
                draw_lines_horizontal();
            break;
        }
        case 1: {
            if(graph_settings.orientation_vertical)
                draw_bars_vertical();
            else
                draw_bars_horizontal();
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
    if(!graph_settings.colored)
    {
        rb->lcd_set_foreground(LCD_DEFAULT_FG);
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
            {
                int32_t color = Q15_DIV(y << 15, LCD_HEIGHT << 15);
                color = Q15_MUL(color, 255 << 15) >> 15;
                rb->lcd_set_foreground(colors[color]);
            }
#       endif
            rb->lcd_drawline(x, LCD_HEIGHT, x, LCD_HEIGHT - y);
        }
    }
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
        hfactor = Q_DIV(LCD_WIDTH << 16, max, 16); /* s15.16 */
    else
        hfactor = Q15_DIV(LCD_WIDTH << 15, max << 15); /* s16.15 */

#ifdef HAVE_LCD_COLOR
    if(!graph_settings.colored)
    {
        rb->lcd_set_foreground(LCD_DEFAULT_FG);
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
                int32_t color = Q15_DIV(x << 15, LCD_WIDTH << 15);
                color = Q15_MUL(color, 255 << 15) >> 15;
                rb->lcd_set_foreground(colors[color]);
            }
#       endif
            rb->lcd_drawline(0, y, x, y);
        }
    }
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
#ifdef HAVE_LCD_COLOR
    graph_settings.colored = false; /* doesn't do anything yet*/
#endif

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
