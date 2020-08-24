/* putchar( int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>

int putchar( int c )
{
    return fputc( c, stdout );
}
