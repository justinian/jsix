/* strxfrm( char *, const char *, size_t )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <string.h>
#include <locale.h>

size_t strxfrm( char * restrict s1, const char * restrict s2, size_t n )
{
    size_t len = strlen( s2 );
    if ( len < n )
    {
        /* Cannot use strncpy() here as the filling of s1 with '\0' is not part
           of the spec.
        */
        /* FIXME: The code below became invalid when we started doing *real* locales... */
        /*while ( n-- && ( *s1++ = _PDCLIB_lc_collate[(unsigned char)*s2++].collation ) );*/
        while ( n-- && ( *s1++ = (unsigned char)*s2++ ) );
    }
    return len;
}
