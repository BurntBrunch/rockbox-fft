/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: arm7 and later wide math functions

 ********************************************************************/
#ifdef CPU_ARM

#if !defined(_V_WIDE_MATH) && !defined(_LOW_ACCURACY_)
#define _V_WIDE_MATH

static inline int32_t MULT32(int32_t x, int32_t y) {
  int lo,hi;
  asm volatile("smull\t%0, %1, %2, %3"
               : "=&r"(lo),"=&r"(hi)
               : "r"(x),"r"(y) );
  return(hi);
}

static inline int32_t MULT31(int32_t x, int32_t y) {
  return MULT32(x,y)<<1;
}

static inline int32_t MULT31_SHIFT15(int32_t x, int32_t y) {
  int lo,hi;
  asm volatile("smull   %0, %1, %2, %3\n\t"
               "movs    %0, %0, lsr #15\n\t"
               "adc %1, %0, %1, lsl #17\n\t"
               : "=&r"(lo),"=&r"(hi)
               : "r"(x),"r"(y)
               : "cc" );
  return(hi);
}

#define XPROD32(a, b, t, v, x, y) \
{ \
  long l; \
  asm(  "smull  %0, %1, %3, %5\n\t" \
    "rsb    %2, %6, #0\n\t" \
    "smlal  %0, %1, %4, %6\n\t" \
    "smull  %0, %2, %3, %2\n\t" \
    "smlal  %0, %2, %4, %5" \
    : "=&r" (l), "=&r" (x), "=&r" (y) \
        : "r" ((a)), "r" ((b)), "r" ((t)), "r" ((v)) ); \
}

static inline void XPROD31(int32_t  a, int32_t  b,
               int32_t  t, int32_t  v,
               int32_t *x, int32_t *y)
{
  int x1, y1, l;
  asm(  "smull  %0, %1, %3, %5\n\t"
    "rsb    %2, %6, #0\n\t"
    "smlal  %0, %1, %4, %6\n\t"
    "smull  %0, %2, %3, %2\n\t"
    "smlal  %0, %2, %4, %5"
    : "=&r" (l), "=&r" (x1), "=&r" (y1)
    : "r" (a), "r" (b), "r" (t), "r" (v) );
  *x = x1 << 1;
  *y = y1 << 1;
}

static inline void XNPROD31(int32_t  a, int32_t  b,
                int32_t  t, int32_t  v,
                int32_t *x, int32_t *y)
{
  int x1, y1, l;
  asm(  "smull  %0, %1, %3, %5\n\t"
    "rsb    %2, %4, #0\n\t"
    "smlal  %0, %1, %2, %6\n\t"
    "smull  %0, %2, %4, %5\n\t"
    "smlal  %0, %2, %3, %6"
    : "=&r" (l), "=&r" (x1), "=&r" (y1)
    : "r" (a), "r" (b), "r" (t), "r" (v) );
  *x = x1 << 1;
  *y = y1 << 1;
}

#define XPROD31_R(_a, _b, _t, _v, _x, _y)\
{\
  int x1, y1, l;\
  asm(  "smull  %0, %1, %5, %3\n\t"\
    "rsb    %2, %3, #0\n\t"\
    "smlal  %0, %1, %6, %4\n\t"\
    "smull  %0, %2, %6, %2\n\t"\
    "smlal  %0, %2, %5, %4"\
    : "=&r" (l), "=&r" (x1), "=&r" (y1)\
    : "r" (_a), "r" (_b), "r" (_t), "r" (_v) );\
  _x = x1 << 1;\
  _y = y1 << 1;\
}

#define XNPROD31_R(_a, _b, _t, _v, _x, _y)\
{\
  int x1, y1, l;\
  asm(  "smull  %0, %1, %5, %3\n\t"\
    "rsb    %2, %4, #0\n\t"\
    "smlal  %0, %1, %6, %2\n\t"\
    "smull  %0, %2, %5, %4\n\t"\
    "smlal  %0, %2, %6, %3"\
    : "=&r" (l), "=&r" (x1), "=&r" (y1)\
    : "r" (_a), "r" (_b), "r" (_t), "r" (_v) );\
  _x = x1 << 1;\
  _y = y1 << 1;\
}

#ifndef _V_VECT_OPS
#define _V_VECT_OPS

/* asm versions of vector operations for block.c, window.c */
static inline
void vect_add(int32_t *x, int32_t *y, int n)
{
  while (n>=4) {
    asm volatile ("ldmia %[x], {r0, r1, r2, r3};"
                  "ldmia %[y]!, {r4, r5, r6, r7};"
                  "add r0, r0, r4;"
                  "add r1, r1, r5;"
                  "add r2, r2, r6;"
                  "add r3, r3, r7;"
                  "stmia %[x]!, {r0, r1, r2, r3};"
                  : [x] "+r" (x), [y] "+r" (y)
                  : : "r0", "r1", "r2", "r3",
                  "r4", "r5", "r6", "r7",
                  "memory");
    n -= 4;
  }
  /* add final elements */
  while (n>0) {
    *x++ += *y++;
    n--;
  }
}

static inline
void vect_copy(int32_t *x, int32_t *y, int n)
{
  while (n>=4) {
    asm volatile ("ldmia %[y]!, {r0, r1, r2, r3};"
                  "stmia %[x]!, {r0, r1, r2, r3};"
                  : [x] "+r" (x), [y] "+r" (y)
                  : : "r0", "r1", "r2", "r3",
                  "memory");
    n -= 4;
  }
  /* copy final elements */
  while (n>0) {
    *x++ = *y++;
    n--;
  }
}

static inline
void vect_mult_fw(int32_t *data, int32_t *window, int n)
{
  while (n>=4) {
    asm volatile ("ldmia %[d], {r0, r1, r2, r3};"
                  "ldmia %[w]!, {r4, r5, r6, r7};"
                  "smull r8, r9, r0, r4;"
                  "mov   r0, r9, lsl #1;"
                  "smull r8, r9, r1, r5;"
                  "mov   r1, r9, lsl #1;"
                  "smull r8, r9, r2, r6;"
                  "mov   r2, r9, lsl #1;"
                  "smull r8, r9, r3, r7;"
                  "mov   r3, r9, lsl #1;"
                  "stmia %[d]!, {r0, r1, r2, r3};"
                  : [d] "+r" (data), [w] "+r" (window)
                  : : "r0", "r1", "r2", "r3",
                  "r4", "r5", "r6", "r7", "r8", "r9",
                  "memory" );
    n -= 4;
  }
  while(n>0) {
    *data = MULT31(*data, *window);
    data++;
    window++;
    n--;
  }
}

static inline
void vect_mult_bw(int32_t *data, int32_t *window, int n)
{
  while (n>=4) {
    asm volatile ("ldmia %[d], {r0, r1, r2, r3};"
                  "ldmda %[w]!, {r4, r5, r6, r7};"
                  "smull r8, r9, r0, r7;"
                  "mov   r0, r9, lsl #1;"
                  "smull r8, r9, r1, r6;"
                  "mov   r1, r9, lsl #1;"
                  "smull r8, r9, r2, r5;"
                  "mov   r2, r9, lsl #1;"
                  "smull r8, r9, r3, r4;"
                  "mov   r3, r9, lsl #1;"
                  "stmia %[d]!, {r0, r1, r2, r3};"
                  : [d] "+r" (data), [w] "+r" (window)
                  : : "r0", "r1", "r2", "r3",
                  "r4", "r5", "r6", "r7", "r8", "r9",
                  "memory" );
    n -= 4;
  }
  while(n>0) {
    *data = MULT31(*data, *window);
    data++;
    window--;
    n--;
  }
}

#endif

#endif
/* not used anymore */
/*
#ifndef _V_CLIP_MATH
#define _V_CLIP_MATH

static inline int32_t CLIP_TO_15(int32_t x) {
  int tmp;
  asm volatile("subs    %1, %0, #32768\n\t"
           "movpl   %0, #0x7f00\n\t"
           "orrpl   %0, %0, #0xff\n"
           "adds    %1, %0, #32768\n\t"
           "movmi   %0, #0x8000"
           : "+r"(x),"=r"(tmp)
           :
           : "cc");
  return(x);
}

#endif
*/
#ifndef _V_LSP_MATH_ASM
#define _V_LSP_MATH_ASM



#endif
#endif

