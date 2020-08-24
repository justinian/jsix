/* isdigit( int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <ctype.h>
#include <locale.h>

int isdigit( int c )
{
    return ( c >= _PDCLIB_lc_ctype.digits_low && c <= _PDCLIB_lc_ctype.digits_high );
}
