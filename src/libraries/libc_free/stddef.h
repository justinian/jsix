#pragma once
/** \file stddef.h
  * Common definitions
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <__j6libc/null.h>
#include <__j6libc/size_t.h>
#include <__j6libc/wchar_t.h>

typedef __PTRDIFF_TYPE__ ptrdiff_t;

#if __BIGGEST_ALIGNMENT__ == __SIZEOF_LONG_DOUBLE__
typedef long double max_align_t;
#else
typedef long long max_align_t;
#endif

#define offsetof(x, y) __builtin_offsetof(x, y)

#define NULL ((void*)0)
