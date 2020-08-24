/* setbuf( FILE *, char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>

void setbuf( struct _PDCLIB_file_t * restrict stream, char * restrict buf )
{
    if ( buf == NULL )
    {
        setvbuf( stream, buf, _IONBF, BUFSIZ );
    }
    else
    {
        setvbuf( stream, buf, _IOFBF, BUFSIZ );
    }
}
