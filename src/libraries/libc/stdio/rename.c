/* rename( const char *, const char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include "j6libc/glue.h"

#include <string.h>

extern struct _PDCLIB_file_t * _PDCLIB_filelist;

int rename( const char * old, const char * new )
{
    struct _PDCLIB_file_t * current = _PDCLIB_filelist;
    while ( current != NULL )
    {
        if ( ( current->filename != NULL ) && ( strcmp( current->filename, old ) == 0 ) )
        {
            /* File of that name currently open. Do not rename. */
            return EOF;
        }
        current = current->next;
    }
    return _PDCLIB_rename( old, new );
}
