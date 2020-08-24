/* atol( const char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdlib.h>

long int atol( const char * s )
{
    return (long int) _PDCLIB_atomax( s );
}
