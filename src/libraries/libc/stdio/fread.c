/* fwrite( void *, size_t, size_t, FILE * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <string.h>
#include "j6libc/glue.h"

size_t fread( void * restrict ptr, size_t size, size_t nmemb, struct _PDCLIB_file_t * restrict stream )
{
    char * dest = (char *)ptr;
    size_t nmemb_i;
    if ( _PDCLIB_prepread( stream ) == EOF )
    {
        return 0;
    }
    for ( nmemb_i = 0; nmemb_i < nmemb; ++nmemb_i )
    {
        size_t size_i;
        for ( size_i = 0; size_i < size; ++size_i )
        {
            if ( stream->bufidx == stream->bufend )
            {
                if ( _PDCLIB_fillbuffer( stream ) == EOF )
                {
                    /* Could not read requested data */
                    return nmemb_i;
                }
            }
            dest[ nmemb_i * size + size_i ] = stream->buffer[ stream->bufidx++ ];
        }
    }
    return nmemb_i;
}
