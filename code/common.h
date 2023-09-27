#ifndef _COMMON_H_
#define _COMMON_H_

#include <assert.h>

#define ASSERT(expr) assert((expr))
#define ARRAY_LEN(array) (sizeof((array))/sizeof((array)[0]))
#define OFFSET_OF(type, value) (&(((type *)0)->value))

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

#define true  1
#define false 0

#endif /* _COMMON_H_ */
