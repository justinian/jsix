/* tmpnam( char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include "j6libc/glue.h"

#include <string.h>

char * tmpnam( char * s )
{
    static char filename[ L_tmpnam ];
    FILE * file = tmpfile();
    if ( s == NULL )
    {
        s = filename;
    }
    strcpy( s, file->filename );
    fclose( file );
    return s;
}
