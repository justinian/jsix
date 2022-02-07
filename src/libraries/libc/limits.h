#pragma once
/** \file limits.h
  * Sizes of integer types
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#define CHAR_BIT     __CHAR_BIT__

#define SCHAR_MAX    __SCHAR_MAX__
#define SCHAR_MIN    ((-__SCHAR_MAX__) - 1)
#define UCHAR_MAX    __UCHAR_MAX__

#if ((char)-1) < 0
// char is signed
#define CHAR_MAX     SCHAR_MAX
#define CHAR_MIN     SCHAR_MIN
#else
// char is unsigned
#define CHAR_MAX     UCHAR_MAX
#define CHAR_MIN     0
#endif

#define MB_LEN_MAX

#define SHRT_MAX     __SHRT_MAX__
#define SHRT_MIN     ((-__SHRT_MAX__) - 1)

#if __SIZEOF_SHORT__ == 1
#define USHRT_MAX    __UINT8_MAX__
#elif __SIZEOF_SHORT__ == 2
#define USHRT_MAX    __UINT16_MAX__
#elif __SIZEOF_SHORT__ == 4
#define USHRT_MAX    __UINT32_MAX__
#endif

#define INT_MAX      __INT_MAX__
#define INT_MIN      ((-__INT_MAX__) - 1)

#if __SIZEOF_INT__ == 1
#define UINT_MAX     __UINT8_MAX__
#elif __SIZEOF_INT__ == 2
#define UINT_MAX     __UINT16_MAX__
#elif __SIZEOF_INT__ == 4
#define UINT_MAX     __UINT32_MAX__
#elif __SIZEOF_INT__ == 8
#define UINT_MAX     __UINT64_MAX__
#endif

#define LONG_MAX     __LONG_MAX__
#define LONG_MIN     ((-__LONG_MAX__) - 1)

#if __SIZEOF_LONG__ == 1
#define ULONG_MAX    __UINT8_MAX__
#elif __SIZEOF_LONG__ == 2
#define ULONG_MAX    __UINT16_MAX__
#elif __SIZEOF_LONG__ == 4
#define ULONG_MAX    __UINT32_MAX__
#elif __SIZEOF_LONG__ == 8
#define ULONG_MAX    __UINT64_MAX__
#elif __SIZEOF_LONG__ == 16
#define ULONG_MAX    __UINT128_MAX__
#endif

#define LLONG_MAX    __LONG_LONG_MAX__
#define LLONG_MIN    ((-__LONG_LONG_MAX__) - 1)

#if __SIZEOF_LONG_LONG__ == 1
#define ULLONG_MAX   __UINT8_MAX__
#elif __SIZEOF_LONG_LONG__ == 2
#define ULLONG_MAX   __UINT16_MAX__
#elif __SIZEOF_LONG_LONG__ == 4
#define ULLONG_MAX   __UINT32_MAX__
#elif __SIZEOF_LONG_LONG__ == 8
#define ULLONG_MAX   __UINT64_MAX__
#elif __SIZEOF_LONG_LONG__ == 16
#define ULLONG_MAX   __UINT128_MAX__
#endif

