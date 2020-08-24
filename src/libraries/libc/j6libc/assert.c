/* _PDCLIB_assert( const char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "j6libc/aux.h"

void _PDCLIB_assert( const char * const message1, const char * const function, const char * const message2 )
{
    fputs( message1, stderr );
    fputs( function, stderr );
    fputs( message2, stderr );
    abort();
}
