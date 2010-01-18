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

#include "jpeg_decoder.h"

/* for portability of below JPEG code */
#define MEMSET(p,v,c) rb->memset(p,v,c)
#define MEMCPY(d,s,c) rb->memcpy(d,s,c)
#define INLINE static inline
#define ENDIAN_SWAP16(n) n /* only for poor little endian machines */

/**************** begin JPEG code ********************/

INLINE unsigned range_limit(int value)
{
#if CONFIG_CPU == SH7034
    unsigned tmp;
    asm (  /* Note: Uses knowledge that only low byte of result is used */
        "mov     #-128,%[t]  \n"
        "sub     %[t],%[v]   \n"  /* value -= -128; equals value += 128; */
        "extu.b  %[v],%[t]   \n"
        "cmp/eq  %[v],%[t]   \n"  /* low byte == whole number ? */
        "bt      1f          \n"  /* yes: no overflow */
        "cmp/pz  %[v]        \n"  /* overflow: positive? */
        "subc    %[v],%[v]   \n"  /* %[r] now either 0 or 0xffffffff */
    "1:                      \n"
        : /* outputs */
        [v]"+r"(value),
        [t]"=&r"(tmp)
    );
    return value;
#elif defined(CPU_COLDFIRE)
    asm (  /* Note: Uses knowledge that only the low byte of the result is used */
        "add.l   #128,%[v]   \n"  /* value += 128; */
        "cmp.l   #255,%[v]   \n"  /* overflow? */
        "bls.b   1f          \n"  /* no: return value */
        "spl.b   %[v]        \n"  /* yes: set low byte to appropriate boundary */
    "1:                      \n"
        : /* outputs */
        [v]"+d"(value)
    );
    return value;
#elif defined(CPU_ARM)
    asm (  /* Note: Uses knowledge that only the low byte of the result is used */
        "add     %[v], %[v], #128    \n"  /* value += 128 */
        "cmp     %[v], #255          \n"  /* out of range 0..255? */
        "mvnhi   %[v], %[v], asr #31 \n"  /* yes: set all bits to ~(sign_bit) */
        : /* outputs */
        [v]"+r"(value)
    );
    return value;
#else
    value += 128;

    if ((unsigned)value <= 255)
        return value;

    if (value < 0)
        return 0;

    return 255;
#endif
}

/* IDCT implementation */


#define CONST_BITS 13
#define PASS1_BITS 2


/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
* causing a lot of useless floating-point operations at run time.
* To get around this we use the following pre-calculated constants.
* If you change CONST_BITS you may want to add appropriate values.
* (With a reasonable C compiler, you can just rely on the FIX() macro...)
*/
#define FIX_0_298631336  2446 /* FIX(0.298631336) */
#define FIX_0_390180644  3196 /* FIX(0.390180644) */
#define FIX_0_541196100  4433 /* FIX(0.541196100) */
#define FIX_0_765366865  6270 /* FIX(0.765366865) */
#define FIX_0_899976223  7373 /* FIX(0.899976223) */
#define FIX_1_175875602  9633 /* FIX(1.175875602) */
#define FIX_1_501321110 12299 /* FIX(1.501321110) */
#define FIX_1_847759065 15137 /* FIX(1.847759065) */
#define FIX_1_961570560 16069 /* FIX(1.961570560) */
#define FIX_2_053119869 16819 /* FIX(2.053119869) */
#define FIX_2_562915447 20995 /* FIX(2.562915447) */
#define FIX_3_072711026 25172 /* FIX(3.072711026) */



/* Multiply an long variable by an long constant to yield an long result.
* For 8-bit samples with the recommended scaling, all the variable
* and constant values involved are no more than 16 bits wide, so a
* 16x16->32 bit multiply can be used instead of a full 32x32 multiply.
* For 12-bit samples, a full 32-bit multiplication will be needed.
*/
#define MULTIPLY16(var,const)  (((short) (var)) * ((short) (const)))


/* Dequantize a coefficient by multiplying it by the multiplier-table
* entry; produce an int result.  In this module, both inputs and result
* are 16 bits or less, so either int or short multiply will work.
*/
/* #define DEQUANTIZE(coef,quantval)  (((int) (coef)) * (quantval)) */
#define DEQUANTIZE MULTIPLY16

/* Descale and correctly round an int value that's scaled by N bits.
* We assume RIGHT_SHIFT rounds towards minus infinity, so adding
* the fudge factor is correct for either sign of X.
*/
#define DESCALE(x,n) (((x) + (1l << ((n)-1))) >> (n))



/*
* Perform dequantization and inverse DCT on one block of coefficients,
* producing a reduced-size 1x1 output block.
*/
void idct1x1(unsigned char* p_byte, int* inptr, int* quantptr, int skip_line)
{
    (void)skip_line; /* unused */
    *p_byte = range_limit(inptr[0] * quantptr[0] >> 3);
}



/*
* Perform dequantization and inverse DCT on one block of coefficients,
* producing a reduced-size 2x2 output block.
*/
void idct2x2(unsigned char* p_byte, int* inptr, int* quantptr, int skip_line)
{
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;
    unsigned char* outptr;

    /* Pass 1: process columns from input, store into work array. */

    /* Column 0 */
    tmp4 = DEQUANTIZE(inptr[8*0], quantptr[8*0]);
    tmp5 = DEQUANTIZE(inptr[8*1], quantptr[8*1]);

    tmp0 = tmp4 + tmp5;
    tmp2 = tmp4 - tmp5;

    /* Column 1 */
    tmp4 = DEQUANTIZE(inptr[8*0+1], quantptr[8*0+1]);
    tmp5 = DEQUANTIZE(inptr[8*1+1], quantptr[8*1+1]);

    tmp1 = tmp4 + tmp5;
    tmp3 = tmp4 - tmp5;

    /* Pass 2: process 2 rows, store into output array. */

    /* Row 0 */
    outptr = p_byte;

    outptr[0] = range_limit((int) DESCALE(tmp0 + tmp1, 3));
    outptr[1] = range_limit((int) DESCALE(tmp0 - tmp1, 3));

    /* Row 1 */
    outptr = p_byte + skip_line;

    outptr[0] = range_limit((int) DESCALE(tmp2 + tmp3, 3));
    outptr[1] = range_limit((int) DESCALE(tmp2 - tmp3, 3));
}



/*
* Perform dequantization and inverse DCT on one block of coefficients,
* producing a reduced-size 4x4 output block.
*/
void idct4x4(unsigned char* p_byte, int* inptr, int* quantptr, int skip_line)
{
    int tmp0, tmp2, tmp10, tmp12;
    int z1, z2, z3;
    int * wsptr;
    unsigned char* outptr;
    int ctr;
    int workspace[4*4]; /* buffers data between passes */

    /* Pass 1: process columns from input, store into work array. */

    wsptr = workspace;
    for (ctr = 0; ctr < 4; ctr++, inptr++, quantptr++, wsptr++)
    {
        /* Even part */

        tmp0 = DEQUANTIZE(inptr[8*0], quantptr[8*0]);
        tmp2 = DEQUANTIZE(inptr[8*2], quantptr[8*2]);

        tmp10 = (tmp0 + tmp2) << PASS1_BITS;
        tmp12 = (tmp0 - tmp2) << PASS1_BITS;

        /* Odd part */
        /* Same rotation as in the even part of the 8x8 LL&M IDCT */

        z2 = DEQUANTIZE(inptr[8*1], quantptr[8*1]);
        z3 = DEQUANTIZE(inptr[8*3], quantptr[8*3]);

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp0 = DESCALE(z1 + MULTIPLY16(z3, - FIX_1_847759065), CONST_BITS-PASS1_BITS);
        tmp2 = DESCALE(z1 + MULTIPLY16(z2, FIX_0_765366865), CONST_BITS-PASS1_BITS);

        /* Final output stage */

        wsptr[4*0] = (int) (tmp10 + tmp2);
        wsptr[4*3] = (int) (tmp10 - tmp2);
        wsptr[4*1] = (int) (tmp12 + tmp0);
        wsptr[4*2] = (int) (tmp12 - tmp0);
    }

    /* Pass 2: process 4 rows from work array, store into output array. */

    wsptr = workspace;
    for (ctr = 0; ctr < 4; ctr++)
    {
        outptr = p_byte + (ctr*skip_line);
        /* Even part */

        tmp0 = (int) wsptr[0];
        tmp2 = (int) wsptr[2];

        tmp10 = (tmp0 + tmp2) << CONST_BITS;
        tmp12 = (tmp0 - tmp2) << CONST_BITS;

        /* Odd part */
        /* Same rotation as in the even part of the 8x8 LL&M IDCT */

        z2 = (int) wsptr[1];
        z3 = (int) wsptr[3];

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp0 = z1 + MULTIPLY16(z3, - FIX_1_847759065);
        tmp2 = z1 + MULTIPLY16(z2, FIX_0_765366865);

        /* Final output stage */

        outptr[0] = range_limit((int) DESCALE(tmp10 + tmp2,
            CONST_BITS+PASS1_BITS+3));
        outptr[3] = range_limit((int) DESCALE(tmp10 - tmp2,
            CONST_BITS+PASS1_BITS+3));
        outptr[1] = range_limit((int) DESCALE(tmp12 + tmp0,
            CONST_BITS+PASS1_BITS+3));
        outptr[2] = range_limit((int) DESCALE(tmp12 - tmp0,
            CONST_BITS+PASS1_BITS+3));

        wsptr += 4;     /* advance pointer to next row */
    }
}



/*
* Perform dequantization and inverse DCT on one block of coefficients.
*/
void idct8x8(unsigned char* p_byte, int* inptr, int* quantptr, int skip_line)
{
    long tmp0, tmp1, tmp2, tmp3;
    long tmp10, tmp11, tmp12, tmp13;
    long z1, z2, z3, z4, z5;
    int * wsptr;
    unsigned char* outptr;
    int ctr;
    int workspace[64];  /* buffers data between passes */

    /* Pass 1: process columns from input, store into work array. */
    /* Note results are scaled up by sqrt(8) compared to a true IDCT; */
    /* furthermore, we scale the results by 2**PASS1_BITS. */

    wsptr = workspace;
    for (ctr = 8; ctr > 0; ctr--)
    {
    /* Due to quantization, we will usually find that many of the input
    * coefficients are zero, especially the AC terms.  We can exploit this
    * by short-circuiting the IDCT calculation for any column in which all
    * the AC terms are zero.  In that case each output is equal to the
    * DC coefficient (with scale factor as needed).
    * With typical images and quantization tables, half or more of the
    * column DCT calculations can be simplified this way.
    */

        if ((inptr[8*1] | inptr[8*2] | inptr[8*3]
           | inptr[8*4] | inptr[8*5] | inptr[8*6] | inptr[8*7]) == 0)
        {
            /* AC terms all zero */
            int dcval = DEQUANTIZE(inptr[8*0], quantptr[8*0]) << PASS1_BITS;

            wsptr[8*0] = wsptr[8*1] = wsptr[8*2] = wsptr[8*3] = wsptr[8*4]
                       = wsptr[8*5] = wsptr[8*6] = wsptr[8*7] = dcval;
            inptr++;      /* advance pointers to next column */
            quantptr++;
            wsptr++;
            continue;
        }

        /* Even part: reverse the even part of the forward DCT. */
        /* The rotator is sqrt(2)*c(-6). */

        z2 = DEQUANTIZE(inptr[8*2], quantptr[8*2]);
        z3 = DEQUANTIZE(inptr[8*6], quantptr[8*6]);

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp2 = z1 + MULTIPLY16(z3, - FIX_1_847759065);
        tmp3 = z1 + MULTIPLY16(z2, FIX_0_765366865);

        z2 = DEQUANTIZE(inptr[8*0], quantptr[8*0]);
        z3 = DEQUANTIZE(inptr[8*4], quantptr[8*4]);

        tmp0 = (z2 + z3) << CONST_BITS;
        tmp1 = (z2 - z3) << CONST_BITS;

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
           transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively. */

        tmp0 = DEQUANTIZE(inptr[8*7], quantptr[8*7]);
        tmp1 = DEQUANTIZE(inptr[8*5], quantptr[8*5]);
        tmp2 = DEQUANTIZE(inptr[8*3], quantptr[8*3]);
        tmp3 = DEQUANTIZE(inptr[8*1], quantptr[8*1]);

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = MULTIPLY16(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */

        tmp0 = MULTIPLY16(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp1 = MULTIPLY16(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp2 = MULTIPLY16(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp3 = MULTIPLY16(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 = MULTIPLY16(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
        z2 = MULTIPLY16(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
        z3 = MULTIPLY16(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
        z4 = MULTIPLY16(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

        wsptr[8*0] = (int) DESCALE(tmp10 + tmp3, CONST_BITS-PASS1_BITS);
        wsptr[8*7] = (int) DESCALE(tmp10 - tmp3, CONST_BITS-PASS1_BITS);
        wsptr[8*1] = (int) DESCALE(tmp11 + tmp2, CONST_BITS-PASS1_BITS);
        wsptr[8*6] = (int) DESCALE(tmp11 - tmp2, CONST_BITS-PASS1_BITS);
        wsptr[8*2] = (int) DESCALE(tmp12 + tmp1, CONST_BITS-PASS1_BITS);
        wsptr[8*5] = (int) DESCALE(tmp12 - tmp1, CONST_BITS-PASS1_BITS);
        wsptr[8*3] = (int) DESCALE(tmp13 + tmp0, CONST_BITS-PASS1_BITS);
        wsptr[8*4] = (int) DESCALE(tmp13 - tmp0, CONST_BITS-PASS1_BITS);

        inptr++; /* advance pointers to next column */
        quantptr++;
        wsptr++;
    }

    /* Pass 2: process rows from work array, store into output array. */
    /* Note that we must descale the results by a factor of 8 == 2**3, */
    /* and also undo the PASS1_BITS scaling. */

    wsptr = workspace;
    for (ctr = 0; ctr < 8; ctr++)
    {
        outptr = p_byte + (ctr*skip_line);
        /* Rows of zeroes can be exploited in the same way as we did with columns.
        * However, the column calculation has created many nonzero AC terms, so
        * the simplification applies less often (typically 5% to 10% of the time).
        * On machines with very fast multiplication, it's possible that the
        * test takes more time than it's worth.  In that case this section
        * may be commented out.
        */

#ifndef NO_ZERO_ROW_TEST
        if ((wsptr[1] | wsptr[2] | wsptr[3]
           | wsptr[4] | wsptr[5] | wsptr[6] | wsptr[7]) == 0)
        {
            /* AC terms all zero */
            unsigned char dcval = range_limit((int) DESCALE((long) wsptr[0],
                PASS1_BITS+3));

            outptr[0] = dcval;
            outptr[1] = dcval;
            outptr[2] = dcval;
            outptr[3] = dcval;
            outptr[4] = dcval;
            outptr[5] = dcval;
            outptr[6] = dcval;
            outptr[7] = dcval;

            wsptr += 8; /* advance pointer to next row */
            continue;
        }
#endif

        /* Even part: reverse the even part of the forward DCT. */
        /* The rotator is sqrt(2)*c(-6). */

        z2 = (long) wsptr[2];
        z3 = (long) wsptr[6];

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp2 = z1 + MULTIPLY16(z3, - FIX_1_847759065);
        tmp3 = z1 + MULTIPLY16(z2, FIX_0_765366865);

        tmp0 = ((long) wsptr[0] + (long) wsptr[4]) << CONST_BITS;
        tmp1 = ((long) wsptr[0] - (long) wsptr[4]) << CONST_BITS;

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
        * transpose is its inverse. i0..i3 are y7,y5,y3,y1 respectively. */

        tmp0 = (long) wsptr[7];
        tmp1 = (long) wsptr[5];
        tmp2 = (long) wsptr[3];
        tmp3 = (long) wsptr[1];

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = MULTIPLY16(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */

        tmp0 = MULTIPLY16(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp1 = MULTIPLY16(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp2 = MULTIPLY16(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp3 = MULTIPLY16(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 = MULTIPLY16(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
        z2 = MULTIPLY16(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
        z3 = MULTIPLY16(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
        z4 = MULTIPLY16(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

        outptr[0] = range_limit((int) DESCALE(tmp10 + tmp3,
            CONST_BITS+PASS1_BITS+3));
        outptr[7] = range_limit((int) DESCALE(tmp10 - tmp3,
            CONST_BITS+PASS1_BITS+3));
        outptr[1] = range_limit((int) DESCALE(tmp11 + tmp2,
            CONST_BITS+PASS1_BITS+3));
        outptr[6] = range_limit((int) DESCALE(tmp11 - tmp2,
            CONST_BITS+PASS1_BITS+3));
        outptr[2] = range_limit((int) DESCALE(tmp12 + tmp1,
            CONST_BITS+PASS1_BITS+3));
        outptr[5] = range_limit((int) DESCALE(tmp12 - tmp1,
            CONST_BITS+PASS1_BITS+3));
        outptr[3] = range_limit((int) DESCALE(tmp13 + tmp0,
            CONST_BITS+PASS1_BITS+3));
        outptr[4] = range_limit((int) DESCALE(tmp13 - tmp0,
            CONST_BITS+PASS1_BITS+3));

        wsptr += 8; /* advance pointer to next row */
    }
}



/* JPEG decoder implementation */

/* Preprocess the JPEG JFIF file */
int process_markers(unsigned char* p_src, long size, struct jpeg* p_jpeg)
{
    unsigned char* p_bytes = p_src;
    int marker_size; /* variable length of marker segment */
    int i, j, n;
    int ret = 0; /* returned flags */

    p_jpeg->p_entropy_end = p_src + size;

    while (p_src < p_bytes + size)
    {
        if (*p_src++ != 0xFF) /* no marker? */
        {
            p_src--; /* it's image data, put it back */
            p_jpeg->p_entropy_data = p_src;
            break; /* exit marker processing */
        }

        switch (*p_src++)
        {
        case 0xFF: /* Fill byte */
            ret |= FILL_FF;
        case 0x00: /* Zero stuffed byte - entropy data */
            p_src--; /* put it back */
            continue;

        case 0xC0: /* SOF Huff  - Baseline DCT */
            {
                ret |= SOF0;
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */
                n = *p_src++; /* sample precision (= 8 or 12) */
                if (n != 8)
                {
                    return(-1); /* Unsupported sample precision */
                }
                p_jpeg->y_size = *p_src++ << 8; /* Highbyte */
                p_jpeg->y_size |= *p_src++; /* Lowbyte */
                p_jpeg->x_size = *p_src++ << 8; /* Highbyte */
                p_jpeg->x_size |= *p_src++; /* Lowbyte */

                n = (marker_size-2-6)/3;
                if (*p_src++ != n || (n != 1 && n != 3))
                {
                    return(-2); /* Unsupported SOF0 component specification */
                }
                for (i=0; i<n; i++)
                {
                    p_jpeg->frameheader[i].ID = *p_src++; /* Component info */
                    p_jpeg->frameheader[i].horizontal_sampling = *p_src >> 4;
                    p_jpeg->frameheader[i].vertical_sampling = *p_src++ & 0x0F;
                    p_jpeg->frameheader[i].quanttable_select = *p_src++;
                    if (p_jpeg->frameheader[i].horizontal_sampling > 2
                     || p_jpeg->frameheader[i].vertical_sampling > 2)
                    return -3; /* Unsupported SOF0 subsampling */
                }
                p_jpeg->blocks = n;
            }
            break;

        case 0xC1: /* SOF Huff  - Extended sequential DCT*/
        case 0xC2: /* SOF Huff  - Progressive DCT*/
        case 0xC3: /* SOF Huff  - Spatial (sequential) lossless*/
        case 0xC5: /* SOF Huff  - Differential sequential DCT*/
        case 0xC6: /* SOF Huff  - Differential progressive DCT*/
        case 0xC7: /* SOF Huff  - Differential spatial*/
        case 0xC8: /* SOF Arith - Reserved for JPEG extensions*/
        case 0xC9: /* SOF Arith - Extended sequential DCT*/
        case 0xCA: /* SOF Arith - Progressive DCT*/
        case 0xCB: /* SOF Arith - Spatial (sequential) lossless*/
        case 0xCD: /* SOF Arith - Differential sequential DCT*/
        case 0xCE: /* SOF Arith - Differential progressive DCT*/
        case 0xCF: /* SOF Arith - Differential spatial*/
            {
                return (-4); /* other DCT model than baseline not implemented */
            }

        case 0xC4: /* Define Huffman Table(s) */
            {
                unsigned char* p_temp;

                ret |= DHT;
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */

                p_temp = p_src;
                while (p_src < p_temp+marker_size-2-17) /* another table */
                {
                    int sum = 0;
                    i = *p_src & 0x0F; /* table index */
                    if (i > 1)
                    {
                        return (-5); /* Huffman table index out of range */
                    }
                    else if (*p_src++ & 0xF0) /* AC table */
                    {
                        for (j=0; j<16; j++)
                        {
                            sum += *p_src;
                            p_jpeg->hufftable[i].huffmancodes_ac[j] = *p_src++;
                        }
                        if(16 + sum > AC_LEN)
                            return -10; /* longer than allowed */

                        for (; j < 16 + sum; j++)
                            p_jpeg->hufftable[i].huffmancodes_ac[j] = *p_src++;
                    }
                    else /* DC table */
                    {
                        for (j=0; j<16; j++)
                        {
                            sum += *p_src;
                            p_jpeg->hufftable[i].huffmancodes_dc[j] = *p_src++;
                        }
                        if(16 + sum > DC_LEN)
                            return -11; /* longer than allowed */

                        for (; j < 16 + sum; j++)
                            p_jpeg->hufftable[i].huffmancodes_dc[j] = *p_src++;
                    }
                } /* while */
                p_src = p_temp+marker_size - 2; /* skip possible residue */
            }
            break;

        case 0xCC: /* Define Arithmetic coding conditioning(s) */
            return(-6); /* Arithmetic coding not supported */

        case 0xD8: /* Start of Image */
        case 0xD9: /* End of Image */
        case 0x01: /* for temp private use arith code */
            break; /* skip parameterless marker */


        case 0xDA: /* Start of Scan */
            {
                ret |= SOS;
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */

                n = (marker_size-2-1-3)/2;
                if (*p_src++ != n || (n != 1 && n != 3))
                {
                    return (-7); /* Unsupported SOS component specification */
                }
                for (i=0; i<n; i++)
                {
                    p_jpeg->scanheader[i].ID = *p_src++;
                    p_jpeg->scanheader[i].DC_select = *p_src >> 4;
                    p_jpeg->scanheader[i].AC_select = *p_src++ & 0x0F;
                }
                p_src += 3; /* skip spectral information */
            }
            break;

        case 0xDB: /* Define quantization Table(s) */
            {
                ret |= DQT;
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */
                n = (marker_size-2)/(QUANT_TABLE_LENGTH+1); /* # of tables */
                for (i=0; i<n; i++)
                {
                    int id = *p_src++; /* ID */
                    if (id >= 4)
                    {
                        return (-8); /* Unsupported quantization table */
                    }
                    /* Read Quantisation table: */
                    for (j=0; j<QUANT_TABLE_LENGTH; j++)
                        p_jpeg->quanttable[id][j] = *p_src++;
                }
            }
            break;

        case 0xDD: /* Define Restart Interval */
            {
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */
                p_jpeg->restart_interval = *p_src++ << 8; /* Highbyte */
                p_jpeg->restart_interval |= *p_src++; /* Lowbyte */
                p_src += marker_size-4; /* skip segment */
            }
            break;

        case 0xDC: /* Define Number of Lines */
        case 0xDE: /* Define Hierarchical progression */
        case 0xDF: /* Expand Reference Component(s) */
        case 0xE0: /* Application Field 0*/
        case 0xE1: /* Application Field 1*/
        case 0xE2: /* Application Field 2*/
        case 0xE3: /* Application Field 3*/
        case 0xE4: /* Application Field 4*/
        case 0xE5: /* Application Field 5*/
        case 0xE6: /* Application Field 6*/
        case 0xE7: /* Application Field 7*/
        case 0xE8: /* Application Field 8*/
        case 0xE9: /* Application Field 9*/
        case 0xEA: /* Application Field 10*/
        case 0xEB: /* Application Field 11*/
        case 0xEC: /* Application Field 12*/
        case 0xED: /* Application Field 13*/
        case 0xEE: /* Application Field 14*/
        case 0xEF: /* Application Field 15*/
        case 0xFE: /* Comment */
            {
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */
                p_src += marker_size-2; /* skip segment */
            }
            break;

        case 0xF0: /* Reserved for JPEG extensions */
        case 0xF1: /* Reserved for JPEG extensions */
        case 0xF2: /* Reserved for JPEG extensions */
        case 0xF3: /* Reserved for JPEG extensions */
        case 0xF4: /* Reserved for JPEG extensions */
        case 0xF5: /* Reserved for JPEG extensions */
        case 0xF6: /* Reserved for JPEG extensions */
        case 0xF7: /* Reserved for JPEG extensions */
        case 0xF8: /* Reserved for JPEG extensions */
        case 0xF9: /* Reserved for JPEG extensions */
        case 0xFA: /* Reserved for JPEG extensions */
        case 0xFB: /* Reserved for JPEG extensions */
        case 0xFC: /* Reserved for JPEG extensions */
        case 0xFD: /* Reserved for JPEG extensions */
        case 0x02: /* Reserved */
        default:
            return (-9); /* Unknown marker */
        } /* switch */
    } /* while */

    return (ret); /* return flags with seen markers */
}


void default_huff_tbl(struct jpeg* p_jpeg)
{
    static const struct huffman_table luma_table =
    {
        {
            0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B
        },
        {
            0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,0x7D,
            0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
            0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,
            0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,
            0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
            0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
            0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
            0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
            0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,
            0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,
            0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
            0xF9,0xFA
        }
    };

    static const struct huffman_table chroma_table =
    {
        {
            0x00,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,
            0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B
        },
        {
            0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,0x02,0x77,
            0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,
            0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,0xF0,
            0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26,
            0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,
            0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,
            0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,0x86,0x87,
            0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,
            0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,
            0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,
            0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
            0xF9,0xFA
        }
    };

    MEMCPY(&p_jpeg->hufftable[0], &luma_table, sizeof(luma_table));
    MEMCPY(&p_jpeg->hufftable[1], &chroma_table, sizeof(chroma_table));

    return;
}

/* Compute the derived values for a Huffman table */
void fix_huff_tbl(int* htbl, struct derived_tbl* dtbl)
{
    int p, i, l, si;
    int lookbits, ctr;
    char huffsize[257];
    unsigned int huffcode[257];
    unsigned int code;

    dtbl->pub = htbl; /* fill in back link */

    /* Figure C.1: make table of Huffman code length for each symbol */
    /* Note that this is in code-length order. */

    p = 0;
    for (l = 1; l <= 16; l++)
    {    /* all possible code length */
        for (i = 1; i <= (int) htbl[l-1]; i++)  /* all codes per length */
            huffsize[p++] = (char) l;
    }
    huffsize[p] = 0;

    /* Figure C.2: generate the codes themselves */
    /* Note that this is in code-length order. */

    code = 0;
    si = huffsize[0];
    p = 0;
    while (huffsize[p])
    {
        while (((int) huffsize[p]) == si)
        {
            huffcode[p++] = code;
            code++;
        }
        code <<= 1;
        si++;
    }

    /* Figure F.15: generate decoding tables for bit-sequential decoding */

    p = 0;
    for (l = 1; l <= 16; l++)
    {
        if (htbl[l-1])
        {
            dtbl->valptr[l] = p; /* huffval[] index of 1st symbol of code length l */
            dtbl->mincode[l] = huffcode[p]; /* minimum code of length l */
            p += htbl[l-1];
            dtbl->maxcode[l] = huffcode[p-1]; /* maximum code of length l */
        }
        else
        {
            dtbl->maxcode[l] = -1;  /* -1 if no codes of this length */
        }
    }
    dtbl->maxcode[17] = 0xFFFFFL; /* ensures huff_DECODE terminates */

    /* Compute lookahead tables to speed up decoding.
    * First we set all the table entries to 0, indicating "too long";
    * then we iterate through the Huffman codes that are short enough and
    * fill in all the entries that correspond to bit sequences starting
    * with that code.
    */

    MEMSET(dtbl->look_nbits, 0, sizeof(dtbl->look_nbits));

    p = 0;
    for (l = 1; l <= HUFF_LOOKAHEAD; l++)
    {
        for (i = 1; i <= (int) htbl[l-1]; i++, p++)
        {
            /* l = current code's length, p = its index in huffcode[] & huffval[]. */
            /* Generate left-justified code followed by all possible bit sequences */
            lookbits = huffcode[p] << (HUFF_LOOKAHEAD-l);
            for (ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--)
            {
                dtbl->look_nbits[lookbits] = l;
                dtbl->look_sym[lookbits] = htbl[16+p];
                lookbits++;
            }
        }
    }
}


/* zag[i] is the natural-order position of the i'th element of zigzag order.
 * If the incoming data is corrupted, decode_mcu could attempt to
 * reference values beyond the end of the array.  To avoid a wild store,
 * we put some extra zeroes after the real entries.
 */
static const int zag[] =
{
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
     0,  0,  0,  0,  0,  0,  0,  0, /* extra entries in case k>63 below */
     0,  0,  0,  0,  0,  0,  0,  0
};

void build_lut(struct jpeg* p_jpeg)
{
    int i;
    fix_huff_tbl(p_jpeg->hufftable[0].huffmancodes_dc,
        &p_jpeg->dc_derived_tbls[0]);
    fix_huff_tbl(p_jpeg->hufftable[0].huffmancodes_ac,
        &p_jpeg->ac_derived_tbls[0]);
    fix_huff_tbl(p_jpeg->hufftable[1].huffmancodes_dc,
        &p_jpeg->dc_derived_tbls[1]);
    fix_huff_tbl(p_jpeg->hufftable[1].huffmancodes_ac,
        &p_jpeg->ac_derived_tbls[1]);

    /* build the dequantization tables for the IDCT (De-ZiZagged) */
    for (i=0; i<64; i++)
    {
        p_jpeg->qt_idct[0][zag[i]] = p_jpeg->quanttable[0][i];
        p_jpeg->qt_idct[1][zag[i]] = p_jpeg->quanttable[1][i];
    }

    for (i=0; i<4; i++)
        p_jpeg->store_pos[i] = i; /* default ordering */

    /* assignments for the decoding of blocks */
    if (p_jpeg->frameheader[0].horizontal_sampling == 2
        && p_jpeg->frameheader[0].vertical_sampling == 1)
    {   /* 4:2:2 */
        p_jpeg->blocks = 4;
        p_jpeg->x_mbl = (p_jpeg->x_size+15) / 16;
        p_jpeg->x_phys = p_jpeg->x_mbl * 16;
        p_jpeg->y_mbl = (p_jpeg->y_size+7) / 8;
        p_jpeg->y_phys = p_jpeg->y_mbl * 8;
        p_jpeg->mcu_membership[0] = 0; /* Y1=Y2=0, U=1, V=2 */
        p_jpeg->mcu_membership[1] = 0;
        p_jpeg->mcu_membership[2] = 1;
        p_jpeg->mcu_membership[3] = 2;
        p_jpeg->tab_membership[0] = 0; /* DC, DC, AC, AC */
        p_jpeg->tab_membership[1] = 0;
        p_jpeg->tab_membership[2] = 1;
        p_jpeg->tab_membership[3] = 1;
        p_jpeg->subsample_x[0] = 1;
        p_jpeg->subsample_x[1] = 2;
        p_jpeg->subsample_x[2] = 2;
        p_jpeg->subsample_y[0] = 1;
        p_jpeg->subsample_y[1] = 1;
        p_jpeg->subsample_y[2] = 1;
    }
    if (p_jpeg->frameheader[0].horizontal_sampling == 1
        && p_jpeg->frameheader[0].vertical_sampling == 2)
    {   /* 4:2:2 vertically subsampled */
        p_jpeg->store_pos[1] = 2; /* block positions are mirrored */
        p_jpeg->store_pos[2] = 1;
        p_jpeg->blocks = 4;
        p_jpeg->x_mbl = (p_jpeg->x_size+7) / 8;
        p_jpeg->x_phys = p_jpeg->x_mbl * 8;
        p_jpeg->y_mbl = (p_jpeg->y_size+15) / 16;
        p_jpeg->y_phys = p_jpeg->y_mbl * 16;
        p_jpeg->mcu_membership[0] = 0; /* Y1=Y2=0, U=1, V=2 */
        p_jpeg->mcu_membership[1] = 0;
        p_jpeg->mcu_membership[2] = 1;
        p_jpeg->mcu_membership[3] = 2;
        p_jpeg->tab_membership[0] = 0; /* DC, DC, AC, AC */
        p_jpeg->tab_membership[1] = 0;
        p_jpeg->tab_membership[2] = 1;
        p_jpeg->tab_membership[3] = 1;
        p_jpeg->subsample_x[0] = 1;
        p_jpeg->subsample_x[1] = 1;
        p_jpeg->subsample_x[2] = 1;
        p_jpeg->subsample_y[0] = 1;
        p_jpeg->subsample_y[1] = 2;
        p_jpeg->subsample_y[2] = 2;
    }
    else if (p_jpeg->frameheader[0].horizontal_sampling == 2
        && p_jpeg->frameheader[0].vertical_sampling == 2)
    {   /* 4:2:0 */
        p_jpeg->blocks = 6;
        p_jpeg->x_mbl = (p_jpeg->x_size+15) / 16;
        p_jpeg->x_phys = p_jpeg->x_mbl * 16;
        p_jpeg->y_mbl = (p_jpeg->y_size+15) / 16;
        p_jpeg->y_phys = p_jpeg->y_mbl * 16;
        p_jpeg->mcu_membership[0] = 0;
        p_jpeg->mcu_membership[1] = 0;
        p_jpeg->mcu_membership[2] = 0;
        p_jpeg->mcu_membership[3] = 0;
        p_jpeg->mcu_membership[4] = 1;
        p_jpeg->mcu_membership[5] = 2;
        p_jpeg->tab_membership[0] = 0;
        p_jpeg->tab_membership[1] = 0;
        p_jpeg->tab_membership[2] = 0;
        p_jpeg->tab_membership[3] = 0;
        p_jpeg->tab_membership[4] = 1;
        p_jpeg->tab_membership[5] = 1;
        p_jpeg->subsample_x[0] = 1;
        p_jpeg->subsample_x[1] = 2;
        p_jpeg->subsample_x[2] = 2;
        p_jpeg->subsample_y[0] = 1;
        p_jpeg->subsample_y[1] = 2;
        p_jpeg->subsample_y[2] = 2;
    }
    else if (p_jpeg->frameheader[0].horizontal_sampling == 1
        && p_jpeg->frameheader[0].vertical_sampling == 1)
    {   /* 4:4:4 */
        /* don't overwrite p_jpeg->blocks */
        p_jpeg->x_mbl = (p_jpeg->x_size+7) / 8;
        p_jpeg->x_phys = p_jpeg->x_mbl * 8;
        p_jpeg->y_mbl = (p_jpeg->y_size+7) / 8;
        p_jpeg->y_phys = p_jpeg->y_mbl * 8;
        p_jpeg->mcu_membership[0] = 0;
        p_jpeg->mcu_membership[1] = 1;
        p_jpeg->mcu_membership[2] = 2;
        p_jpeg->tab_membership[0] = 0;
        p_jpeg->tab_membership[1] = 1;
        p_jpeg->tab_membership[2] = 1;
        p_jpeg->subsample_x[0] = 1;
        p_jpeg->subsample_x[1] = 1;
        p_jpeg->subsample_x[2] = 1;
        p_jpeg->subsample_y[0] = 1;
        p_jpeg->subsample_y[1] = 1;
        p_jpeg->subsample_y[2] = 1;
    }
    else
    {
        /* error */
    }

}


/*
* These functions/macros provide the in-line portion of bit fetching.
* Use check_bit_buffer to ensure there are N bits in get_buffer
* before using get_bits, peek_bits, or drop_bits.
*  check_bit_buffer(state,n,action);
*    Ensure there are N bits in get_buffer; if suspend, take action.
*  val = get_bits(n);
*    Fetch next N bits.
*  val = peek_bits(n);
*    Fetch next N bits without removing them from the buffer.
*  drop_bits(n);
*    Discard next N bits.
* The value N should be a simple variable, not an expression, because it
* is evaluated multiple times.
*/

INLINE void check_bit_buffer(struct bitstream* pb, int nbits)
{
    if (pb->bits_left < nbits)
    {   /* nbits is <= 16, so I can always refill 2 bytes in this case */
        unsigned char byte;

        byte = *pb->next_input_byte++;
        if (byte == 0xFF) /* legal marker can be byte stuffing or RSTm */
        {   /* simplification: just skip the (one-byte) marker code */
            pb->next_input_byte++;
        }
        pb->get_buffer = (pb->get_buffer << 8) | byte;

        byte = *pb->next_input_byte++;
        if (byte == 0xFF) /* legal marker can be byte stuffing or RSTm */
        {   /* simplification: just skip the (one-byte) marker code */
            pb->next_input_byte++;
        }
        pb->get_buffer = (pb->get_buffer << 8) | byte;

        pb->bits_left += 16;
    }
}

INLINE int get_bits(struct bitstream* pb, int nbits)
{
    return ((int) (pb->get_buffer >> (pb->bits_left -= nbits))) & (BIT_N(nbits)-1);
}

INLINE int peek_bits(struct bitstream* pb, int nbits)
{
    return ((int) (pb->get_buffer >> (pb->bits_left - nbits))) & (BIT_N(nbits)-1);
}

INLINE void drop_bits(struct bitstream* pb, int nbits)
{
    pb->bits_left -= nbits;
}

/* re-synchronize to entropy data (skip restart marker) */
void search_restart(struct bitstream* pb)
{
    pb->next_input_byte--; /* we may have overread it, taking 2 bytes */
    /* search for a non-byte-padding marker, has to be RSTm or EOS */
    while (pb->next_input_byte < pb->input_end &&
        (pb->next_input_byte[-2] != 0xFF || pb->next_input_byte[-1] == 0x00))
    {
        pb->next_input_byte++;
    }
    pb->bits_left = 0;
}

/* Figure F.12: extend sign bit. */
#define HUFF_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

static const int extend_test[16] =   /* entry n is 2**(n-1) */
{
    0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000
};

static const int extend_offset[16] = /* entry n is (-1 << n) + 1 */
{
    0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1
};

/* Decode a single value */
INLINE int huff_decode_dc(struct bitstream* bs, struct derived_tbl* tbl)
{
    int nb, look, s, r;

    check_bit_buffer(bs, HUFF_LOOKAHEAD);
    look = peek_bits(bs, HUFF_LOOKAHEAD);
    if ((nb = tbl->look_nbits[look]) != 0)
    {
        drop_bits(bs, nb);
        s = tbl->look_sym[look];
        check_bit_buffer(bs, s);
        r = get_bits(bs, s);
        s = HUFF_EXTEND(r, s);
    }
    else
    {   /*  slow_DECODE(s, HUFF_LOOKAHEAD+1)) < 0); */
        long code;
        nb=HUFF_LOOKAHEAD+1;
        check_bit_buffer(bs, nb);
        code = get_bits(bs, nb);
        while (code > tbl->maxcode[nb])
        {
            code <<= 1;
            check_bit_buffer(bs, 1);
            code |= get_bits(bs, 1);
            nb++;
        }
        if (nb > 16) /* error in Huffman */
        {
            s=0; /* fake a zero, this is most safe */
        }
        else
        {
            s = tbl->pub[16 + tbl->valptr[nb] + ((int) (code - tbl->mincode[nb])) ];
            check_bit_buffer(bs, s);
            r = get_bits(bs, s);
            s = HUFF_EXTEND(r, s);
        }
    } /* end slow decode */
    return s;
}

INLINE int huff_decode_ac(struct bitstream* bs, struct derived_tbl* tbl)
{
    int nb, look, s;

    check_bit_buffer(bs, HUFF_LOOKAHEAD);
    look = peek_bits(bs, HUFF_LOOKAHEAD);
    if ((nb = tbl->look_nbits[look]) != 0)
    {
        drop_bits(bs, nb);
        s = tbl->look_sym[look];
    }
    else
    {   /*  slow_DECODE(s, HUFF_LOOKAHEAD+1)) < 0); */
        long code;
        nb=HUFF_LOOKAHEAD+1;
        check_bit_buffer(bs, nb);
        code = get_bits(bs, nb);
        while (code > tbl->maxcode[nb])
        {
            code <<= 1;
            check_bit_buffer(bs, 1);
            code |= get_bits(bs, 1);
            nb++;
        }
        if (nb > 16) /* error in Huffman */
        {
            s=0; /* fake a zero, this is most safe */
        }
        else
        {
            s = tbl->pub[16 + tbl->valptr[nb] + ((int) (code - tbl->mincode[nb])) ];
        }
    } /* end slow decode */
    return s;
}


#ifdef HAVE_LCD_COLOR

/* JPEG decoder variant for YUV decoding, into 3 different planes */
/*  Note: it keeps the original color subsampling, even if resized. */
int jpeg_decode(struct jpeg* p_jpeg, unsigned char* p_pixel[3],
                int downscale, void (*pf_progress)(int current, int total))
{
    struct bitstream bs; /* bitstream "object" */
    int block[64]; /* decoded DCT coefficients */

    int width, height;
    int skip_line[3]; /* bytes from one line to the next (skip_line) */
    int skip_strip[3], skip_mcu[3]; /* bytes to next DCT row / column */

    int i, x, y; /* loop counter */

    unsigned char* p_line[3] = {p_pixel[0], p_pixel[1], p_pixel[2]};
    unsigned char* p_byte[3]; /* bitmap pointer */

    void (*pf_idct)(unsigned char*, int*, int*, int); /* selected IDCT */
    int k_need; /* AC coefficients needed up to here */
    int zero_need; /* init the block with this many zeros */

    int last_dc_val[3] = {0, 0, 0}; /* or 128 for chroma? */
    int store_offs[4]; /* memory offsets: order of Y11 Y12 Y21 Y22 U V */
    int restart = p_jpeg->restart_interval; /* MCUs until restart marker */

    /* pick the IDCT we want, determine how to work with coefs */
    if (downscale == 1)
    {
        pf_idct = idct8x8;
        k_need = 64; /* all */
        zero_need = 63; /* all */
    }
    else if (downscale == 2)
    {
        pf_idct = idct4x4;
        k_need = 25; /* this far in zig-zag to cover 4*4 */
        zero_need = 27; /* clear this far in linear order */
    }
    else if (downscale == 4)
    {
        pf_idct = idct2x2;
        k_need = 5; /* this far in zig-zag to cover 2*2 */
        zero_need = 9; /* clear this far in linear order */
    }
    else if (downscale == 8)
    {
        pf_idct = idct1x1;
        k_need = 0; /* no AC, not needed */
        zero_need = 0; /* no AC, not needed */
    }
    else return -1; /* not supported */

    /* init bitstream, fake a restart to make it start */
    bs.next_input_byte = p_jpeg->p_entropy_data;
    bs.bits_left = 0;
    bs.input_end = p_jpeg->p_entropy_end;

    width  = p_jpeg->x_phys / downscale;
    height = p_jpeg->y_phys / downscale;
    for (i=0; i<3; i++) /* calculate some strides */
    {
        skip_line[i] = width / p_jpeg->subsample_x[i];
        skip_strip[i] = skip_line[i]
                        * (height / p_jpeg->y_mbl) / p_jpeg->subsample_y[i];
        skip_mcu[i] = width/p_jpeg->x_mbl / p_jpeg->subsample_x[i];
    }

    /* prepare offsets about where to store the different blocks */
    store_offs[p_jpeg->store_pos[0]] = 0;
    store_offs[p_jpeg->store_pos[1]] = 8 / downscale; /* to the right */
    store_offs[p_jpeg->store_pos[2]] = width * 8 / downscale; /* below */
    store_offs[p_jpeg->store_pos[3]] = store_offs[1] + store_offs[2]; /* r+b */

    for(y=0; y<p_jpeg->y_mbl && bs.next_input_byte <= bs.input_end; y++)
    {
        for (i=0; i<3; i++) /* scan line init */
        {
            p_byte[i] = p_line[i];
            p_line[i] += skip_strip[i];
        }
        for (x=0; x<p_jpeg->x_mbl; x++)
        {
            int blkn;

            /* Outer loop handles each block in the MCU */
            for (blkn = 0; blkn < p_jpeg->blocks; blkn++)
            {   /* Decode a single block's worth of coefficients */
                int k = 1; /* coefficient index */
                int s, r; /* huffman values */
                int ci = p_jpeg->mcu_membership[blkn]; /* component index */
                int ti = p_jpeg->tab_membership[blkn]; /* table index */
                struct derived_tbl* dctbl = &p_jpeg->dc_derived_tbls[ti];
                struct derived_tbl* actbl = &p_jpeg->ac_derived_tbls[ti];

                /* Section F.2.2.1: decode the DC coefficient difference */
                s = huff_decode_dc(&bs, dctbl);

                last_dc_val[ci] += s;
                block[0] = last_dc_val[ci]; /* output it (assumes zag[0] = 0) */

                /* coefficient buffer must be cleared */
                MEMSET(block+1, 0, zero_need*sizeof(block[0]));

                /* Section F.2.2.2: decode the AC coefficients */
                for (; k < k_need; k++)
                {
                    s = huff_decode_ac(&bs, actbl);
                    r = s >> 4;
                    s &= 15;

                    if (s)
                    {
                        k += r;
                        check_bit_buffer(&bs, s);
                        r = get_bits(&bs, s);
                        block[zag[k]] = HUFF_EXTEND(r, s);
                    }
                    else
                    {
                        if (r != 15)
                        {
                            k = 64;
                            break;
                        }
                        k += r;
                    }
                }  /* for k */
                /* In this path we just discard the values */
                for (; k < 64; k++)
                {
                    s = huff_decode_ac(&bs, actbl);
                    r = s >> 4;
                    s &= 15;

                    if (s)
                    {
                        k += r;
                        check_bit_buffer(&bs, s);
                        drop_bits(&bs, s);
                    }
                    else
                    {
                        if (r != 15)
                            break;
                        k += r;
                    }
                }  /* for k */

                if (ci == 0)
                {   /* Y component needs to bother about block store */
                    pf_idct(p_byte[0]+store_offs[blkn], block,
                        p_jpeg->qt_idct[ti], skip_line[0]);
                }
                else
                {   /* chroma */
                    pf_idct(p_byte[ci], block, p_jpeg->qt_idct[ti],
                        skip_line[ci]);
                }
            } /* for blkn */
            p_byte[0] += skip_mcu[0]; /* unrolled for (i=0; i<3; i++) loop */
            p_byte[1] += skip_mcu[1];
            p_byte[2] += skip_mcu[2];
            if (p_jpeg->restart_interval && --restart == 0)
            {   /* if a restart marker is due: */
                restart = p_jpeg->restart_interval; /* count again */
                search_restart(&bs); /* align the bitstream */
                last_dc_val[0] = last_dc_val[1] =
                                 last_dc_val[2] = 0; /* reset decoder */
            }
        } /* for x */
        if (pf_progress != NULL)
            pf_progress(y, p_jpeg->y_mbl-1); /* notify about decoding progress */
    } /* for y */

    return 0; /* success */
}
#else /* !HAVE_LCD_COLOR */

/* a JPEG decoder specialized in decoding only the luminance (b&w) */
int jpeg_decode(struct jpeg* p_jpeg, unsigned char* p_pixel[1], int downscale,
                void (*pf_progress)(int current, int total))
{
    struct bitstream bs; /* bitstream "object" */
    int block[64]; /* decoded DCT coefficients */

    int width, height;
    int skip_line; /* bytes from one line to the next (skip_line) */
    int skip_strip, skip_mcu; /* bytes to next DCT row / column */

    int x, y; /* loop counter */

    unsigned char* p_line = p_pixel[0];
    unsigned char* p_byte; /* bitmap pointer */

    void (*pf_idct)(unsigned char*, int*, int*, int); /* selected IDCT */
    int k_need; /* AC coefficients needed up to here */
    int zero_need; /* init the block with this many zeros */

    int last_dc_val = 0;
    int store_offs[4]; /* memory offsets: order of Y11 Y12 Y21 Y22 U V */
    int restart = p_jpeg->restart_interval; /* MCUs until restart marker */

    /* pick the IDCT we want, determine how to work with coefs */
    if (downscale == 1)
    {
        pf_idct = idct8x8;
        k_need = 64; /* all */
        zero_need = 63; /* all */
    }
    else if (downscale == 2)
    {
        pf_idct = idct4x4;
        k_need = 25; /* this far in zig-zag to cover 4*4 */
        zero_need = 27; /* clear this far in linear order */
    }
    else if (downscale == 4)
    {
        pf_idct = idct2x2;
        k_need = 5; /* this far in zig-zag to cover 2*2 */
        zero_need = 9; /* clear this far in linear order */
    }
    else if (downscale == 8)
    {
        pf_idct = idct1x1;
        k_need = 0; /* no AC, not needed */
        zero_need = 0; /* no AC, not needed */
    }
    else return -1; /* not supported */

    /* init bitstream, fake a restart to make it start */
    bs.next_input_byte = p_jpeg->p_entropy_data;
    bs.bits_left = 0;
    bs.input_end = p_jpeg->p_entropy_end;

    width  = p_jpeg->x_phys / downscale;
    height = p_jpeg->y_phys / downscale;
    skip_line = width;
    skip_strip = skip_line * (height / p_jpeg->y_mbl);
    skip_mcu = (width/p_jpeg->x_mbl);

    /* prepare offsets about where to store the different blocks */
    store_offs[p_jpeg->store_pos[0]] = 0;
    store_offs[p_jpeg->store_pos[1]] = 8 / downscale; /* to the right */
    store_offs[p_jpeg->store_pos[2]] = width * 8 / downscale; /* below */
    store_offs[p_jpeg->store_pos[3]] = store_offs[1] + store_offs[2]; /* r+b */

    for(y=0; y<p_jpeg->y_mbl && bs.next_input_byte <= bs.input_end; y++)
    {
        p_byte = p_line;
        p_line += skip_strip;
        for (x=0; x<p_jpeg->x_mbl; x++)
        {
            int blkn;

            /* Outer loop handles each block in the MCU */
            for (blkn = 0; blkn < p_jpeg->blocks; blkn++)
            {   /* Decode a single block's worth of coefficients */
                int k = 1; /* coefficient index */
                int s, r; /* huffman values */
                int ci = p_jpeg->mcu_membership[blkn]; /* component index */
                int ti = p_jpeg->tab_membership[blkn]; /* table index */
                struct derived_tbl* dctbl = &p_jpeg->dc_derived_tbls[ti];
                struct derived_tbl* actbl = &p_jpeg->ac_derived_tbls[ti];

                /* Section F.2.2.1: decode the DC coefficient difference */
                s = huff_decode_dc(&bs, dctbl);

                if (ci == 0) /* only for Y component */
                {
                    last_dc_val += s;
                    block[0] = last_dc_val; /* output it (assumes zag[0] = 0) */

                    /* coefficient buffer must be cleared */
                    MEMSET(block+1, 0, zero_need*sizeof(block[0]));

                    /* Section F.2.2.2: decode the AC coefficients */
                    for (; k < k_need; k++)
                    {
                        s = huff_decode_ac(&bs, actbl);
                        r = s >> 4;
                        s &= 15;

                        if (s)
                        {
                            k += r;
                            check_bit_buffer(&bs, s);
                            r = get_bits(&bs, s);
                            block[zag[k]] = HUFF_EXTEND(r, s);
                        }
                        else
                        {
                            if (r != 15)
                            {
                                k = 64;
                                break;
                            }
                            k += r;
                        }
                    }  /* for k */
                }
                /* In this path we just discard the values */
                for (; k < 64; k++)
                {
                    s = huff_decode_ac(&bs, actbl);
                    r = s >> 4;
                    s &= 15;

                    if (s)
                    {
                        k += r;
                        check_bit_buffer(&bs, s);
                        drop_bits(&bs, s);
                    }
                    else
                    {
                        if (r != 15)
                            break;
                        k += r;
                    }
                }  /* for k */

                if (ci == 0)
                {   /* only for Y component */
                    pf_idct(p_byte+store_offs[blkn], block, p_jpeg->qt_idct[ti],
                        skip_line);
                }
            } /* for blkn */
            p_byte += skip_mcu;
            if (p_jpeg->restart_interval && --restart == 0)
            {   /* if a restart marker is due: */
                restart = p_jpeg->restart_interval; /* count again */
                search_restart(&bs); /* align the bitstream */
                last_dc_val = 0; /* reset decoder */
            }
        } /* for x */
        if (pf_progress != NULL)
            pf_progress(y, p_jpeg->y_mbl-1); /* notify about decoding progress */
    } /* for y */

    return 0; /* success */
}
#endif /* !HAVE_LCD_COLOR */

/**************** end JPEG code ********************/
