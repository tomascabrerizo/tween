#ifndef _COMMON_H_
#define _COMMON_H_

#include <assert.h>

#define ASSERT(expr) assert((expr))
#define ARRAY_LEN(array) (sizeof((array))/sizeof((array)[0]))
#define OFFSET_OF(type, value) (&(((type *)0)->value))

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX3(a, b, c) MAX(MAX(a, b), c)
#define MIN3(a, b, c) MIN(MIN(a, b), c)

#define ABS(a) ((a) >= 0 ? (a) : (-(a)))
#define CLAMP(value, min, max) MAX(MIN(value, max), min)

typedef unsigned long long u64;
typedef unsigned int       u32;
typedef unsigned short     u16;
typedef unsigned char      u8;

typedef long long          s64;
typedef int                s32;
typedef short              s16;
typedef char               s8;

typedef float              f32;
typedef double             f64;

typedef unsigned int       b32;
typedef unsigned char      b8;

#endif /* _COMMON_H_ */
