/* atexit( void (*)( void ) )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdlib.h>

extern void (*_PDCLIB_exitstack[])( void );
extern size_t _PDCLIB_exitptr;

int atexit( void (*func)( void ) )
{
    if ( _PDCLIB_exitptr == 0 )
    {
        return -1;
    }
    else
    {
        _PDCLIB_exitstack[ --_PDCLIB_exitptr ] = func;
        return 0;
    }
}
