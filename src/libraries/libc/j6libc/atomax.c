/* _PDCLIB_atomax( const char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <string.h>
#include <ctype.h>

intmax_t _PDCLIB_atomax( const char * s )
{
    intmax_t rc = 0;
    char sign = '+';
    const char * x;
    /* TODO: In other than "C" locale, additional patterns may be defined     */
    while ( isspace( *s ) ) ++s;
    if ( *s == '+' ) ++s;
    else if ( *s == '-' ) sign = *(s++);
    /* TODO: Earlier version was missing tolower() but was not caught by tests */
    while ( ( x = memchr( _PDCLIB_digits, tolower(*(s++)), 10 ) ) != NULL )
    {
        rc = rc * 10 + ( x - _PDCLIB_digits );
    }
    return ( sign == '+' ) ? rc : -rc;
}
