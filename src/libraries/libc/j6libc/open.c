/* _PDCLIB_open( const char * const, int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

/* This is an example implementation of _PDCLIB_open() fit for use with POSIX
   kernels.
*/

#include "j6libc/glue.h"

int _PDCLIB_open( const char * const filename, unsigned int mode )
{
	_PDCLIB_errno = _PDCLIB_ERROR;
    return -1;
}
