/* vprintf( const char *, va_list )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdarg.h>

int vprintf( const char * restrict format, _PDCLIB_va_list arg )
{
    return vfprintf( stdout, format, arg );
}
