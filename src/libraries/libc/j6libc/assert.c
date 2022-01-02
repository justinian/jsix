/* _PDCLIB_assert( const char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "j6libc/aux.h"

void _PDCLIB_assert( const char * const message, const char * const function, const char * const file, unsigned line )
{
    fprintf( stderr, "Assertion failed: %s, function %s, file %s, line %d.%s",
            message, function, file, line, _PDCLIB_endl );
    abort();
}
