/* _Exit( int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdlib.h>
#include <stdio.h>
#include "j6libc/glue.h"

void _Exit( int status )
{
    /* TODO: Flush and close open streams. Remove tmpfile() files. Make this
       called on process termination automatically.
    */
    _PDCLIB_Exit( status );
}
