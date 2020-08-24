/* _PDCLIB_strtox_prelim( const char *, char *, int * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <ctype.h>
#include <stddef.h>
#include <string.h>

const char * _PDCLIB_strtox_prelim( const char * p, char * sign, int * base )
{
    /* skipping leading whitespace */
    while ( isspace( *p ) ) ++p;
    /* determining / skipping sign */
    if ( *p != '+' && *p != '-' ) *sign = '+';
    else *sign = *(p++);
    /* determining base */
    if ( *p == '0' )
    {
        ++p;
        if ( ( *base == 0 || *base == 16 ) && ( *p == 'x' || *p == 'X' ) )
        {
            *base = 16;
            ++p;
            /* catching a border case here: "0x" followed by a non-digit should
               be parsed as the unprefixed zero.
               We have to "rewind" the parsing; having the base set to 16 if it
               was zero previously does not hurt, as the result is zero anyway.
            */
            if ( memchr( _PDCLIB_digits, tolower(*p), *base ) == NULL )
            {
                p -= 2;
            }
        }
        else if ( *base == 0 )
        {
            *base = 8;
        }
        else
        {
            --p;
        }
    }
    else if ( ! *base )
    {
        *base = 10;
    }
    return ( ( *base >= 2 ) && ( *base <= 36 ) ) ? p : NULL;
}
