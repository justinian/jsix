/* fgetc( FILE * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include "j6libc/glue.h"

int fgetc( struct _PDCLIB_file_t * stream )
{
    if ( _PDCLIB_prepread( stream ) == EOF )
    {
        return EOF;
    }
    if ( stream->ungetidx > 0 )
    {
        return (unsigned char)stream->ungetbuf[ --(stream->ungetidx) ];
    }
    return (unsigned char)stream->buffer[stream->bufidx++];
}
