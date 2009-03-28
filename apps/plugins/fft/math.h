#ifndef __MATH_H_
#define __MATH_H_

#include <math.h>
#include "lib/fixedpoint.h"

#define Q15_MUL(a, b) (( (int64_t) (a) * (int64_t) (b) + (1 << 14)) >> 15)
#define Q15_DIV(a, b) (int32_t)( (( ((int64_t) (a)) << 15 ) + (b)/2 ) / (b) )
#define float_q15(a) (int32_t)( ((float)(a)) *(1<<15))
#define DEGREESPERRADIAN 57.295779506


#endif
