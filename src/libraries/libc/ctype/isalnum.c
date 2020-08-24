/* isalnum( int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <ctype.h>
#include <locale.h>

int isalnum( int c )
{
    return ( isdigit( c ) || isalpha( c ) );
}
