/* _PDCLIB_is_leap( int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include "j6libc/int.h"

int _PDCLIB_is_leap( int year_offset )
{
    /* year given as offset from 1900, matching tm.tm_year in <time.h> */
    long long year = year_offset + 1900ll;
    return ( ( year % 4 ) == 0 && ( ( year % 25 ) != 0 || ( year % 400 ) == 0 ) );
}
