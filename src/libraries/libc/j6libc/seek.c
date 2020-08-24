/* int64_t _PDCLIB_seek( FILE *, int64_t, int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

/* This is an example implementation of _PDCLIB_seek() fit for use with POSIX
   kernels.
 */

#include <stdio.h>
#include "j6libc/glue.h"

int64_t _PDCLIB_seek( struct _PDCLIB_file_t * stream, int64_t offset, int whence )
{
	_PDCLIB_errno = _PDCLIB_ERROR;
    return EOF;
}
