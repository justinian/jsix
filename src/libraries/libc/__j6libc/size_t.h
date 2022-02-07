#pragma once
/** \file j6libc/size_t.h
  * Internal definition of size_t
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

typedef __SIZE_TYPE__    size_t;
#define SIZE_MAX         __SIZE_MAX__

#if __SIZE_WIDTH__ == 32
#define SIZE_C( x )      x ## __UINT32_C_SUFFIX__
#elif __SIZE_WIDTH__ == 64
#define SIZE_C( x )      x ## __UINT64_C_SUFFIX__
#elif __SIZE_WIDTH__ == 128
#define SIZE_C( x )      x ## __UINT128_C_SUFFIX__
#else
#error Unsupported size_t width
#endif
