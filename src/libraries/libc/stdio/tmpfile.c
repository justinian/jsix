/* tmpfile( void )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include "j6libc/glue.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

extern struct _PDCLIB_file_t * _PDCLIB_filelist;

/* This is an example implementation of tmpfile() fit for use with POSIX
   kernels.
*/
struct _PDCLIB_file_t * tmpfile( void )
{
	return NULL;
}
