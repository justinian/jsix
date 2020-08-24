/* fgets( char *, int, FILE * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include "j6libc/glue.h"

char * fgets( char * restrict s, int size, struct _PDCLIB_file_t * restrict stream )
{
    char * dest = s;
    if ( size == 0 )
    {
        return NULL;
    }
    if ( size == 1 )
    {
        *s = '\0';
        return s;
    }
    if ( _PDCLIB_prepread( stream ) == EOF )
    {
        return NULL;
    }
    while ( ( ( *dest++ = stream->buffer[stream->bufidx++] ) != '\n' ) && --size > 0 )
    {
        if ( stream->bufidx == stream->bufend )
        {
            if ( _PDCLIB_fillbuffer( stream ) == EOF )
            {
                /* In case of error / EOF before a character is read, this
                   will lead to a \0 be written anyway. Since the results
                   are "indeterminate" by definition, this does not hurt.
                */
                break;
            }
        }
    }
    *dest = '\0';
    return ( dest == s ) ? NULL : s;
}
