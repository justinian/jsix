/* ispunct( int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <ctype.h>
#include <locale.h>

int ispunct( int c )
{
    return ( _PDCLIB_lc_ctype.entry[c].flags & _PDCLIB_CTYPE_PUNCT );
}
