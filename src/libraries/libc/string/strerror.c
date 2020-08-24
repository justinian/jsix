/* strerror( int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <string.h>
#include <locale.h>

/* TODO: Doing this via a static array is not the way to do it. */
char * strerror( int errnum )
{
    if ( errnum >= _PDCLIB_ERRNO_MAX )
    {
        return (char *)"Unknown error";
    }
    else
    {
        return _PDCLIB_lc_messages.errno_texts[errnum];
    }
}
