#pragma once
/* Sizes of integer types <limits.h>

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include "j6libc/int.h"

/* TODO: Defined to 1 as multibyte characters are not supported yet. */
#define MB_LEN_MAX 1

#define SCHAR_MAX  __SCHAR_MAX__
#define SCHAR_MIN  ((-SCHAR_MAX) - 1)
#define UCHAR_MAX  (((unsigned char)SCHAR_MAX << 1) | 1)

#define CHAR_BIT   __CHAR_BIT__
#define CHAR_MAX   SCHAR_MAX
#define CHAR_MIN   SCHAR_MIN

#define SHRT_MAX   __SHRT_MAX__
#define SHRT_MIN   ((-SHRT_MAX) - 1)
#define USHRT_MAX  (((unsigned short)SHRT_MAX << 1) | 1)

#define INT_MAX    __INT_MAX__
#define INT_MIN    ((-INT_MAX) - 1)
#define UINT_MAX   (((unsigned int)INT_MAX << 1) | 1)

#define LONG_MAX   __LONG_MAX__
#define LONG_MIN   ((-LONG_MAX) - 1)
#define ULONG_MAX  (((unsigned long)LONG_MAX << 1) | 1)

#define LLONG_MAX  __LONG_LONG_MAX__
#define LLONG_MIN  ((-LLONG_MAX) - 1)
#define ULLONG_MAX (((unsigned long long)LLONG_MAX << 1) | 1)

