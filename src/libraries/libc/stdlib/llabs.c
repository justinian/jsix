/* llabs( long int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdlib.h>

long long int llabs( long long int j )
{
    return ( j >= 0 ) ? j : -j;
}
