#pragma once
/* Diagnostics <assert.h>

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include "j6libc/aux.h"
#include "j6libc/cpp.h"

CPP_CHECK_BEGIN

void _PDCLIB_assert( const char * const, const char * const, const char * const, unsigned );

/* If NDEBUG is set, assert() is a null operation. */
#undef assert

#ifdef NDEBUG
#define assert( ignore ) ( (void) 0 )
#else
#define assert( expression ) ( ( expression ) ? (void) 0 \
        : _PDCLIB_assert( #expression, __func__, __FILE__, __LINE__ ) )
#endif

CPP_CHECK_END
