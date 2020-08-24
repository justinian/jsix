#pragma once
/* Diagnostics <assert.h>

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include "j6libc/aux.h"
#include "j6libc/cpp.h"

CPP_CHECK_BEGIN

void _PDCLIB_assert( const char * const, const char * const, const char * const );

/* If NDEBUG is set, assert() is a null operation. */
#undef assert

#ifdef NDEBUG
#define assert( ignore ) ( (void) 0 )
#else
#define assert( expression ) ( ( expression ) ? (void) 0 \
        : _PDCLIB_assert( "Assertion failed: " #expression \
                          ", function ", __func__, \
                          ", file " __FILE__ \
                          ", line " _PDCLIB_symbol2string( __LINE__ ) \
                          "." _PDCLIB_endl ) )
#endif

CPP_CHECK_END
