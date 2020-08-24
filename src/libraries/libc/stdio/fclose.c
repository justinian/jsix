/* fclose( FILE * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdlib.h>
#include "j6libc/glue.h"

extern struct _PDCLIB_file_t * _PDCLIB_filelist;

int fclose( struct _PDCLIB_file_t * stream )
{
    struct _PDCLIB_file_t * current = _PDCLIB_filelist;
    struct _PDCLIB_file_t * previous = NULL;
    /* Checking that the FILE handle is actually one we had opened before. */
    while ( current != NULL )
    {
        if ( stream == current )
        {
            /* Flush buffer */
            if ( stream->status & _PDCLIB_FWRITE )
            {
                if ( _PDCLIB_flushbuffer( stream ) == EOF )
                {
                    /* Flush failed, errno already set */
                    return EOF;
                }
            }
            /* Close handle */
            _PDCLIB_close( stream->handle );
            /* Remove stream from list */
            if ( previous != NULL )
            {
                previous->next = stream->next;
            }
            else
            {
                _PDCLIB_filelist = stream->next;
            }
            /* Delete tmpfile() */
            if ( stream->status & _PDCLIB_DELONCLOSE )
            {
                remove( stream->filename );
            }
            /* Free user buffer (SetVBuf allocated) */
            if ( stream->status & _PDCLIB_FREEBUFFER )
            {
                free( stream->buffer );
            }
            /* Free stream */
            if ( ! ( stream->status & _PDCLIB_STATIC ) )
            {
                free( stream );
            }
            return 0;
        }
        previous = current;
        current = current->next;
    }
    /* See the comments on implementation-defined errno values in
       <config.h>.
    */
    _PDCLIB_errno = _PDCLIB_ERROR;
    return -1;
}
