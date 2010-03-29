/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for AS3514 and compatible audio codec
 *
 * Copyright (c) 2007 Daniel Ankers
 * Copyright (c) 2007 Christian Gmeiner
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
#include "cpu.h"
#include "debug.h"
#include "system.h"
#include "audio.h"
#include "sound.h"

#include "audiohw.h"
#include "i2s.h"
#include "ascodec.h"

/*
 * This drivers supports:
 * as3514 , as used in the PP targets
 * as3517 , as used in the as3525 targets
 * as3543 , as used in the as3525v2 targets
 */

/* AMS Sansas based on the AS3525 use the LINE2 input for the analog radio
   signal instead of LINE1 */
#if CONFIG_CPU == AS3525
#define LINE_INPUT 2
#else
#define LINE_INPUT 1
#endif

const struct sound_settings_info audiohw_settings[] = {
    [SOUND_VOLUME]        = {"dB",   0,   1, -74,   6, -25},
    /* HAVE_SW_TONE_CONTROLS */
    [SOUND_BASS]          = {"dB",   0,   1, -24,  24,   0},
    [SOUND_TREBLE]        = {"dB",   0,   1, -24,  24,   0},
    [SOUND_BALANCE]       = {"%",    0,   1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",     0,   1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",    0,   5,   0, 250, 100},
#ifdef HAVE_RECORDING
    [SOUND_MIC_GAIN]      = {"dB",   1,   1,   0,  39,  23},
    [SOUND_LEFT_GAIN]     = {"dB",   1,   1,   0,  31,  23},
    [SOUND_RIGHT_GAIN]    = {"dB",   1,   1,   0,  31,  23},
#endif
};

/* Shadow registers */
static struct as3514_info
{
    int     vol_r;       /* Cached volume level (R) */
    int     vol_l;       /* Cached volume level (L) */
    uint8_t regs[AS3514_NUM_AUDIO_REGS]; /* 8-bit registers */
} as3514;

/*
 * little helper method to set register values.
 * With the help of as3514.regs, we minimize i2c
 * traffic.
 */
static void as3514_write(unsigned int reg, unsigned int value)
{
    if (ascodec_write(reg, value) != 2)
    {
        DEBUGF("as3514 error reg=0x%02x", reg);
    }

    if (reg < ARRAYLEN(as3514.regs))
    {
        as3514.regs[reg] = value;
    }
    else
    {
        DEBUGF("as3514 error reg=0x%02x", reg);
    }
}

/* Helpers to set/clear bits */
static void as3514_set(unsigned int reg, unsigned int bits)
{
    as3514_write(reg, as3514.regs[reg] | bits);
}

static void as3514_clear(unsigned int reg, unsigned int bits)
{
    as3514_write(reg, as3514.regs[reg] & ~bits);
}

static void as3514_write_masked(unsigned int reg, unsigned int bits,
                                unsigned int mask)
{
    as3514_write(reg, (as3514.regs[reg] & ~mask) | (bits & mask));
}

/* convert tenth of dB volume to master volume register value */
int tenthdb2master(int db)
{
    /* +6 to -73.5dB in 1.5dB steps == 53 levels */
    if (db < VOLUME_MIN) {
        return 0x0;
    } else if (db >= VOLUME_MAX) {
        return 0x35;
    } else {
        return((db-VOLUME_MIN)/15); /* VOLUME_MIN is negative */
    }
}

int sound_val2phys(int setting, int value)
{
    int result;

    switch(setting)
    {
#if defined(HAVE_RECORDING)
    case SOUND_LEFT_GAIN:
    case SOUND_RIGHT_GAIN:
    case SOUND_MIC_GAIN:
        result = (value - 23) * 15;
        break;
#endif

    default:
        result = value;
        break;
    }

    return result;
}

/*
 * Initialise the PP I2C and I2S.
 */
void audiohw_preinit(void)
{
    unsigned int i;

    /* read all reg values */
    for (i = 0; i < ARRAYLEN(as3514.regs); i++)
    {
        as3514.regs[i] = ascodec_read(i);
    }

    /* Set ADC off, mixer on, DAC on, line out off, line in off, mic off */

    /* Turn on SUM, DAC */
    as3514_write(AS3514_AUDIOSET1, AUDIOSET1_DAC_on | AUDIOSET1_SUM_on);

    /* Set BIAS on, DITH on, AGC on, IBR_DAC max, LSP_LP on, IBR_LSP min */
    as3514_write(AS3514_AUDIOSET2,
                 AUDIOSET2_IBR_DAC_0 | AUDIOSET2_LSP_LP |
                 AUDIOSET2_IBR_LSP_50);

/* AMS Sansas based on the AS3525 need HPCM enabled, otherwise they output the
   L-R signal on both L and R headphone outputs instead of normal stereo.
   Turning it off saves a little power on targets that don't need it. */
#if (CONFIG_CPU == AS3525)
    /* Set HPCM on, ZCU on */
    as3514_write(AS3514_AUDIOSET3, 0);
#else
    /* Set HPCM off, ZCU on */
    as3514_write(AS3514_AUDIOSET3, AUDIOSET3_HPCM_off);
#endif

#if CONFIG_CPU != AS3525v2
    /* Mute and disable speaker */
    as3514_write(AS3514_LSP_OUT_R, LSP_OUT_R_SP_OVC_TO_256MS | 0x00);
    as3514_write(AS3514_LSP_OUT_L, LSP_OUT_L_SP_MUTE | 0x00);
#else
    as3514_clear(AS3543_DAC_IF, 0x80);
    as3514_set(AS3514_LINE_IN1_R, 1<<6); // Select Line-in 2
#endif

#if CONFIG_CPU != AS3525v2
    /* Set headphone over-current to 0, Min volume */
    as3514_write(AS3514_HPH_OUT_R,
                 HPH_OUT_R_HP_OVC_TO_0MS | 0x00);
#else
    as3514_write(AS3514_HPH_OUT_R, (0<<7) /* out */ | HPH_OUT_R_HP_OUT_DAC |
                                  0x00);
#endif
    /* Headphone ON, MUTE, Min volume */
    as3514_write(AS3514_HPH_OUT_L,
                 HPH_OUT_L_HP_ON | HPH_OUT_L_HP_MUTE | 0x00);

#ifdef PHILIPS_SA9200
    /* LRCK 8-23kHz (there are audible clicks while reading the ADC otherwise) */
    as3514_write(AS3514_PLLMODE, PLLMODE_LRCK_8_23);
#else
    /* LRCK 24-48kHz */
    as3514_write(AS3514_PLLMODE, PLLMODE_LRCK_24_48);
#endif

    /* DAC_Mute_off */
    as3514_set(AS3514_DAC_L, DAC_L_DAC_MUTE_off);

    /* M1_Sup_off */
    as3514_set(AS3514_MIC1_L, MIC1_L_M1_SUP_off);
    /* M2_Sup_off */
    as3514_set(AS3514_MIC2_L, MIC2_L_M2_SUP_off);
}

void audiohw_postinit(void)
{
    /* wait until outputs have stabilized */
    sleep(HZ/4);

#ifdef CPU_PP
    ascodec_suppressor_on(false);
#endif

    audiohw_mute(false);
}

void audiohw_set_master_vol(int vol_l, int vol_r)
{
    unsigned int hph_r, hph_l;
    unsigned int mix_l, mix_r;

    /* keep track of current setting */
    as3514.vol_l = vol_l;
    as3514.vol_r = vol_r;

    /* We combine the mixer channel volume range with the headphone volume
       range - keep first stage as loud as possible */
    if (vol_r <= 0x16) {
        mix_r = vol_r;
        hph_r = 0;
    } else {
        mix_r = 0x16;
        hph_r = vol_r - 0x16;
    }

    if (vol_l <= 0x16) {
        mix_l = vol_l;
        hph_l = 0;
    } else {
        mix_l = 0x16;
        hph_l = vol_l - 0x16;
    }

    as3514_write_masked(AS3514_DAC_R, mix_r, AS3514_VOL_MASK);
    as3514_write_masked(AS3514_DAC_L, mix_l, AS3514_VOL_MASK);
#if defined(HAVE_RECORDING) || defined(HAVE_FMRADIO_IN)
    as3514_write_masked((LINE_INPUT == 1) ? AS3514_LINE_IN1_R : 
                                            AS3514_LINE_IN2_R,
                        mix_r, AS3514_VOL_MASK);
    as3514_write_masked((LINE_INPUT == 1) ? AS3514_LINE_IN1_L : 
                                            AS3514_LINE_IN2_L,
                        mix_l, AS3514_VOL_MASK);
#endif
    as3514_write_masked(AS3514_HPH_OUT_R, hph_r, AS3514_VOL_MASK);
    as3514_write_masked(AS3514_HPH_OUT_L, hph_l, AS3514_VOL_MASK);
}

void audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    as3514_write_masked(AS3514_LINE_OUT_R, vol_r, AS3514_VOL_MASK);
    as3514_write_masked(AS3514_LINE_OUT_L, vol_l, AS3514_VOL_MASK);
}

void audiohw_mute(bool mute)
{
    if (mute) {
        as3514_set(AS3514_HPH_OUT_L, HPH_OUT_L_HP_MUTE);
#if CONFIG_CPU == AS3525v2
        as3514_set(AS3543_DAC_IF, 0x80);
#endif

    } else {
        as3514_clear(AS3514_HPH_OUT_L, HPH_OUT_L_HP_MUTE);
#if CONFIG_CPU == AS3525v2
        as3514_clear(AS3543_DAC_IF, 0x80);
#endif
    }
}

/* Nice shutdown of AS3514 audio codec */
void audiohw_close(void)
{
    /* mute headphones */
    audiohw_mute(true);

#ifdef CPU_PP
    ascodec_suppressor_on(true);
#endif

    /* turn on common */
    as3514_clear(AS3514_AUDIOSET3, AUDIOSET3_HPCM_off);

    /* turn off everything */
    as3514_clear(AS3514_HPH_OUT_L, HPH_OUT_L_HP_ON);
    as3514_write(AS3514_AUDIOSET1, 0x0);

#if CONFIG_CPU == AS3525v2
    as3514_set(AS3543_DAC_IF, 0x80);
#endif

    /* Allow caps to discharge */
    sleep(HZ/4);
}

void audiohw_set_frequency(int fsel)
{
    (void)fsel;
}

#if defined(HAVE_RECORDING)
void audiohw_enable_recording(bool source_mic)
{
    if (source_mic) {
        /* ADCmux = Stereo Microphone */
        as3514_write_masked(AS3514_ADC_R, ADC_R_ADCMUX_ST_MIC,
                            ADC_R_ADCMUX);

        /* MIC1_on, others off */
        as3514_write_masked(AS3514_AUDIOSET1, AUDIOSET1_MIC1_on, 
                            AUDIOSET1_INPUT_MASK);

        /* M1_AGC_off */
        as3514_clear(AS3514_MIC1_R, MIC1_R_M1_AGC_off);
    } else {
        /* ADCmux = Line_IN1 or Line_IN2 */
        as3514_write_masked(AS3514_ADC_R,
                            (LINE_INPUT == 1) ? ADC_R_ADCMUX_LINE_IN1 :
                                                ADC_R_ADCMUX_LINE_IN2,
                            ADC_R_ADCMUX);

        /* LIN1_or LIN2 on, rest off */
        as3514_write_masked(AS3514_AUDIOSET1,
                            (LINE_INPUT == 1) ? AUDIOSET1_LIN1_on :
                                                AUDIOSET1_LIN2_on,
                            AUDIOSET1_INPUT_MASK);
    }

    /* ADC_Mute_off */
    as3514_set(AS3514_ADC_L, ADC_L_ADC_MUTE_off);
    /* ADC_on */
    as3514_set(AS3514_AUDIOSET1, AUDIOSET1_ADC_on);
}

void audiohw_disable_recording(void)
{
    /* ADC_Mute_on */
    as3514_clear(AS3514_ADC_L, ADC_L_ADC_MUTE_off);

    /* ADC_off, all input sources off */
    as3514_clear(AS3514_AUDIOSET1, AUDIOSET1_ADC_on | AUDIOSET1_INPUT_MASK);
}

/**
 * Set recording volume
 *
 * Line in   : 0 .. 23 .. 31 =>
               Volume -34.5 .. +00.0 .. +12.0 dB
 * Mic (left): 0 .. 23 .. 39 =>
 *             Volume -34.5 .. +00.0 .. +24.0 dB
 *
 */
void audiohw_set_recvol(int left, int right, int type)
{
    switch (type)
    {
    case AUDIO_GAIN_MIC:
    {
        /* Combine MIC gains seamlessly with ADC levels */
        unsigned int mic1_r;

        if (left >= 36) {
            /* M1_Gain = +40db, ADR_Vol = +7.5dB .. +12.0 dB =>
               +19.5 dB .. +24.0 dB */
            left -= 8;
            mic1_r = MIC1_R_M1_GAIN_40DB;
        } else if (left >= 32) {
            /* M1_Gain = +34db, ADR_Vol = +7.5dB .. +12.0 dB =>
               +13.5 dB .. +18.0 dB */
            left -= 4; 
            mic1_r = MIC1_R_M1_GAIN_34DB;
        } else {
            /* M1_Gain = +28db, ADR_Vol = -34.5dB .. +12.0 dB =>
               -34.5 dB .. +12.0 dB */
            mic1_r = MIC1_R_M1_GAIN_28DB;
        }

        right = left;

        as3514_write_masked(AS3514_MIC1_R, mic1_r, MIC1_R_M1_GAIN);
        break;
        }
    case AUDIO_GAIN_LINEIN:
        break;
    default:
        return;
    }

    as3514_write_masked(AS3514_ADC_R, right, AS3514_VOL_MASK);
    as3514_write_masked(AS3514_ADC_L, left, AS3514_VOL_MASK);
}
#endif /* HAVE_RECORDING */

#if defined(HAVE_RECORDING) || defined(HAVE_FMRADIO_IN)
/**
 * Enable line in analog monitoring
 *
 */
void audiohw_set_monitor(bool enable)
{
    if (enable) {
        /* select either LIN1 or LIN2 */
        as3514_write_masked(AS3514_AUDIOSET1,
                            (LINE_INPUT == 1) ? 
                            AUDIOSET1_LIN1_on : AUDIOSET1_LIN2_on,
                            AUDIOSET1_LIN1_on | AUDIOSET1_LIN2_on);
        as3514_set((LINE_INPUT == 1) ? AS3514_LINE_IN1_R : AS3514_LINE_IN2_R,
                   LINE_IN1_R_LI1R_MUTE_off);
        as3514_set((LINE_INPUT == 1) ? AS3514_LINE_IN1_L : AS3514_LINE_IN2_L,
                   LINE_IN1_L_LI1L_MUTE_off);

#if CONFIG_CPU == AS3525v2
        as3514_write_masked(AS3514_HPH_OUT_R,
                            HPH_OUT_R_HP_OUT_LINE, HPH_OUT_R_HP_OUT_MASK);
#endif
    }
    else {
        /* turn off both LIN1 and LIN2 */
        as3514_clear(AS3514_LINE_IN1_R, LINE_IN1_R_LI1R_MUTE_off);
        as3514_clear(AS3514_LINE_IN1_L, LINE_IN1_L_LI1L_MUTE_off);
#if CONFIG_CPU != AS3525v2  /* not in as3543 */
        as3514_clear(AS3514_LINE_IN2_R, LINE_IN2_R_LI2R_MUTE_off);
        as3514_clear(AS3514_LINE_IN2_L, LINE_IN2_L_LI2L_MUTE_off);
#else
        as3514_write_masked(AS3514_HPH_OUT_R,
                            HPH_OUT_R_HP_OUT_DAC, HPH_OUT_R_HP_OUT_MASK);
#endif
        as3514_clear(AS3514_AUDIOSET1, AUDIOSET1_LIN1_on | AUDIOSET1_LIN2_on);
    }
}
#endif /* HAVE_RECORDING || HAVE_FMRADIO_IN */


