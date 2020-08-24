/* fgetpos( FILE * , fpos_t * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>

int fgetpos( struct _PDCLIB_file_t * restrict stream, struct _PDCLIB_fpos_t * restrict pos )
{
    pos->offset = stream->pos.offset + stream->bufidx - stream->ungetidx;
    pos->status = stream->pos.status;
    /* TODO: Add mbstate. */
    return 0;
}
