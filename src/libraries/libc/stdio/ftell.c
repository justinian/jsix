/* ftell( FILE * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <limits.h>

long int ftell( struct _PDCLIB_file_t * stream )
{
    /* ftell() must take into account:
       - the actual *physical* offset of the file, i.e. the offset as recognized
         by the operating system (and stored in stream->pos.offset); and
       - any buffers held by PDCLib, which
         - in case of unwritten buffers, count in *addition* to the offset; or
         - in case of unprocessed pre-read buffers, count in *substraction* to
           the offset. (Remember to count ungetidx into this number.)
       Conveniently, the calculation ( ( bufend - bufidx ) + ungetidx ) results
       in just the right number in both cases:
         - in case of unwritten buffers, ( ( 0 - unwritten ) + 0 )
           i.e. unwritten bytes as negative number
         - in case of unprocessed pre-read, ( ( preread - processed ) + unget )
           i.e. unprocessed bytes as positive number.
       That is how the somewhat obscure return-value calculation works.
    */
    /*  If offset is too large for return type, report error instead of wrong
        offset value.
    */
    /* TODO: Check what happens when ungetc() is called on a stream at offset 0 */
    if ( ( stream->pos.offset - stream->bufend ) > ( LONG_MAX - ( stream->bufidx - stream->ungetidx ) ) )
    {
        /* integer overflow */
        _PDCLIB_errno = _PDCLIB_ERANGE;
        return -1;
    }
    return (long int)( stream->pos.offset - ( ( (int)stream->bufend - (int)stream->bufidx ) + stream->ungetidx ) );
}
