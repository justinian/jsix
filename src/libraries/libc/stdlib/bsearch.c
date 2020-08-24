/* bsearch( const void *, const void *, size_t, size_t, int(*)( const void *, const void * ) )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdlib.h>

void * bsearch( const void * key, const void * base, size_t nmemb, size_t size, int (*compar)( const void *, const void * ) )
{
    const void * pivot;
    int rc;
    size_t corr;
    while ( nmemb )
    {
        /* algorithm needs -1 correction if remaining elements are an even number. */
        corr = nmemb % 2;
        nmemb /= 2;
        pivot = (const char *)base + (nmemb * size);
        rc = compar( key, pivot );
        if ( rc > 0 )
        {
            base = (const char *)pivot + size;
            /* applying correction */
            nmemb -= ( 1 - corr );
        }
        else if ( rc == 0 )
        {
            return (void *)pivot;
        }
    }
    return NULL;
}
