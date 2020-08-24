/* scanf( const char *, ... )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdarg.h>

int scanf( const char * restrict format, ... )
{
    va_list ap;
    va_start( ap, format );
    return vfscanf( stdin, format, ap );
}
