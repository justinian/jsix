#pragma once
/* Common definitions <stddef.h>

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include "j6libc/int.h"
#include "j6libc/null.h"
#include "j6libc/max_align_t.h"
#include "j6libc/size_t.h"
#include "j6libc/wchar_t.h"

typedef __PTRDIFF_TYPE__ ptrdiff_t;

#if ! __has_include("__stddef_max_align_t.h")
typedef long double      max_align_t;
#endif

#ifndef offsetof
#define offsetof( type, member ) _PDCLIB_offsetof( type, member )
#endif
