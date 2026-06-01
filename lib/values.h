/* 
 * values.h - Compatibility header for old-style values.h on modern systems
 *
 * This file provides the definitions that would be in the now-obsolete
 * values.h header used in older Unix systems.
 */

#ifndef _VALUES_H
#define _VALUES_H

#include <limits.h>
#include <float.h>

/* Integer limits */
#ifndef BITSPERBYTE
#define BITSPERBYTE CHAR_BIT
#endif

#ifndef BITS
#define BITS(type)  (BITSPERBYTE * (int)sizeof(type))
#endif

#ifndef HIBITS
#define HIBITS      ((short)(1 << (BITS(short) - 1)))
#endif

#ifndef HIBITL
#define HIBITL      (1L << (BITS(long) - 1))
#endif

/* Maximum and minimum values */
#ifndef MAXSHORT
#define MAXSHORT    SHRT_MAX
#endif

#ifndef MAXINT
#define MAXINT      INT_MAX
#endif

#ifndef MAXLONG
#define MAXLONG     LONG_MAX
#endif

#ifndef MINSHORT
#define MINSHORT    SHRT_MIN
#endif

#ifndef MININT
#define MININT      INT_MIN
#endif

#ifndef MINLONG
#define MINLONG     LONG_MIN
#endif

/* Floating point values */
#ifndef MAXDOUBLE
#define MAXDOUBLE   DBL_MAX
#endif

#ifndef MAXFLOAT
#define MAXFLOAT    FLT_MAX
#endif

#ifndef MINDOUBLE
#define MINDOUBLE   DBL_MIN
#endif

#ifndef MINFLOAT
#define MINFLOAT    FLT_MIN
#endif

#ifndef DMINEXP
#define DMINEXP     DBL_MIN_EXP
#endif

#ifndef FMINEXP
#define FMINEXP     FLT_MIN_EXP
#endif

#ifndef DMAXEXP
#define DMAXEXP     DBL_MAX_EXP
#endif

#ifndef FMAXEXP
#define FMAXEXP     FLT_MAX_EXP
#endif

#endif                          /* _VALUES_H */
