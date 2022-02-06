#pragma once
/* Integer types <stdint.h>

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

/* 7.18.2 Limits of specified-width integer types */

#include <j6libc/int.h>
#include <j6libc/int_widths.h>

#ifdef __cplusplus
#ifndef __STDC_LIMIT_MACROS
#define _PDCLIB_NO_LIMIT_MACROS
#endif
#endif

#ifndef _PDCLIB_NO_LIMIT_MACROS

/* 7.18.2.1 Limits of exact-width integer types */

#define INT8_MAX  __INT8_MAX__
#define INT8_MIN  ((-INT8_MAX) - 1)
#define UINT8_MAX __UINT8_MAX__

#define INT16_MAX  __INT16_MAX__
#define INT16_MIN  ((-INT16_MAX) - 1)
#define UINT16_MAX __UINT16_MAX__

#define INT32_MAX  __INT32_MAX__
#define INT32_MIN  ((-INT32_MAX) - 1)
#define UINT32_MAX __UINT32_MAX__

#define INT64_MAX  __INT64_MAX__
#define INT64_MIN  ((-INT64_MAX) - 1)
#define UINT64_MAX __UINT64_MAX__

/* 7.18.2.2 Limits of minimum-width integer types */

/* For the standard widths, least and exact types are equivalent.
   You are allowed to add more types here, e.g. int_least24_t.
*/

#define INT_LEAST8_MAX  __INT_LEAST8_MAX__
#define INT_LEAST8_MIN  ((-INT_LEAST8_MAX) - 1)
#define UINT_LEAST8_MAX __UINT_LEAST8_MAX__

#define INT_LEAST16_MAX  __INT_LEAST16_MAX__
#define INT_LEAST16_MIN  ((-INT_LEAST16_MAX) - 1)
#define UINT_LEAST16_MAX __UINT_LEAST16_MAX__

#define INT_LEAST32_MAX  __INT_LEAST32_MAX__
#define INT_LEAST32_MIN  ((-INT_LEAST32_MAX) - 1)
#define UINT_LEAST32_MAX __UINT_LEAST32_MAX__

#define INT_LEAST64_MAX  __INT_LEAST64_MAX__
#define INT_LEAST64_MIN  ((-INT_LEAST64_MAX) - 1)
#define UINT_LEAST64_MAX __UINT_LEAST64_MAX__

/* 7.18.2.3 Limits of fastest minimum-width integer types */

#define INT_FAST8_MAX  __INT_FAST8_MAX__
#define INT_FAST8_MIN  ((-INT_FAST8_MAX) - 1)
#define UINT_FAST8_MAX __UINT_FAST8_MAX__

#define INT_FAST16_MAX  __INT_FAST16_MAX__
#define INT_FAST16_MIN  ((-INT_FAST16_MAX) - 1)
#define UINT_FAST16_MAX __UINT_FAST16_MAX__

#define INT_FAST32_MAX  __INT_FAST32_MAX__
#define INT_FAST32_MIN  ((-INT_FAST32_MAX) - 1)
#define UINT_FAST32_MAX __UINT_FAST32_MAX__

#define INT_FAST64_MAX  __INT_FAST64_MAX__
#define INT_FAST64_MIN  ((-INT_FAST64_MAX) - 1)
#define UINT_FAST64_MAX __UINT_FAST64_MAX__

/* 7.18.2.4 Limits of integer types capable of holding object pointers */

#define INTPTR_MAX  __INTPTR_MAX__
#define INTPTR_MIN  ((-INTPTR_MAX) - 1)
#define UINTPTR_MAX __UINTPTR_MAX__

/* 7.18.2.5 Limits of greatest-width integer types */

#define INTMAX_MAX  __INTMAX_MAX__
#define INTMAX_MIN  ((-INTMAX_MAX) - 1)
#define UINTMAX_MAX __UINTMAX_MAX__

/* 7.18.3 Limits of other integer types */

#define PTRDIFF_MAX __PTRDIFF_MAX__
#define PTRDIFF_MIN ((-PTRDIFF_MAX) - 1)

#define SIG_ATOMIC_MAX __SIG_ATOMIC_MAX__
#define SIG_ATOMIC_MIN ((-SIG_ATOMIC_MAX) - 1)

#define SIZE_MAX __SIZE_MAX__

#define WCHAR_MAX __WCHAR_MAX__
#define WCHAR_MIN ((-WCHAR_MAX) - 1)

#define WINT_MAX __WINT_MAX__
#define WINT_MIN ((-WINT_MAX) - 1)

#endif

/* 7.18.4 Macros for integer constants */

#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define _PDCLIB_NO_CONSTANT_MACROS
#endif
#endif

#ifndef _PDCLIB_NO_CONSTANT_MACROS

/* 7.18.4.1 Macros for minimum-width integer constants */

/* As the minimum-width types - for the required widths of 8, 16, 32, and 64
   bits - are expressed in terms of the exact-width types, the mechanism for
   these macros is to append the literal of that exact-width type to the macro
   parameter.
   This is considered a hack, as the author is not sure his understanding of
   the requirements of this macro is correct. Any input appreciated.
*/

/* Expand to an integer constant of specified value and type int_leastN_t */

#define INT8_C( value )  _PDCLIB_concat( value, __INT8_C_SUFFIX__ )
#define INT16_C( value ) _PDCLIB_concat( value, __INT16_C_SUFFIX__ )
#define INT32_C( value ) _PDCLIB_concat( value, __INT32_C_SUFFIX__ )
#define INT64_C( value ) _PDCLIB_concat( value, __INT64_C_SUFFIX__ )

/* Expand to an integer constant of specified value and type uint_leastN_t */

#define UINT8_C( value )  _PDCLIB_concat( value, __UINT8_C_SUFFIX__ )
#define UINT16_C( value ) _PDCLIB_concat( value, __UINT16_C_SUFFIX__ )
#define UINT32_C( value ) _PDCLIB_concat( value, __UINT32_C_SUFFIX__ )
#define UINT64_C( value ) _PDCLIB_concat( value, __UINT64_C_SUFFIX__ )

/* 7.18.4.2 Macros for greatest-width integer constants */

/* Expand to an integer constant of specified value and type intmax_t */
#define INTMAX_C( value ) _PDCLIB_concat( value, __INTMAX_C_SUFFIX__ )

/* Expand to an integer constant of specified value and type uintmax_t */
#define UINTMAX_C( value ) _PDCLIB_concat( value, __UINTMAX_C_SUFFIX__ )

#endif
