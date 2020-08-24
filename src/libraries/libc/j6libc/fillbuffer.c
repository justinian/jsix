/* _PDCLIB_fillbuffer( struct _PDCLIB_file_t * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

/* This is an example implementation of _PDCLIB_fillbuffer() fit for
   use with POSIX kernels.
*/

#include <stdio.h>
#include "j6libc/glue.h"

int _PDCLIB_fillbuffer( struct _PDCLIB_file_t * stream )
{
	_PDCLIB_errno = _PDCLIB_ERROR;
	stream->status |= _PDCLIB_ERRORFLAG;
	return EOF;
}
