/* strtoll( const char *, char * *, int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>

long long int strtoll( const char * s, char ** endptr, int base )
{
    long long int rc;
    char sign = '+';
    const char * p = _PDCLIB_strtox_prelim( s, &sign, &base );
    if ( base < 2 || base > 36 ) return 0;
    if ( sign == '+' )
    {
        rc = (long long int)_PDCLIB_strtox_main( &p, (unsigned)base, (uintmax_t)LLONG_MAX, (uintmax_t)( LLONG_MAX / base ), (int)( LLONG_MAX % base ), &sign );
    }
    else
    {
        rc = (long long int)_PDCLIB_strtox_main( &p, (unsigned)base, (uintmax_t)LLONG_MIN, (uintmax_t)( LLONG_MIN / -base ), (int)( -( LLONG_MIN % base ) ), &sign );
    }
    if ( endptr != NULL ) *endptr = ( p != NULL ) ? (char *) p : (char *) s;
    return ( sign == '+' ) ? rc : -rc;
}
