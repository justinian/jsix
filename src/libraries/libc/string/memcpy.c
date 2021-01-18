/* memcpy( void *, const void *, size_t )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <string.h>

void * memcpy( void * restrict s1, const void * restrict s2, size_t n )
{
    char * dest = (char *) s1;
    const char * src = (const char *) s2;

	if (((uintptr_t)src & 7) == ((uintptr_t)dest & 7)) {
		while (((uintptr_t)src & 7) && n--)
			*dest++ = *src++;

		const uint64_t *srcq = (const uint64_t*)src;
		uint64_t *destq = (uint64_t*)dest;
		while (n >= 8) {
			*destq++ = *srcq++;
			n -= 8;
		}

		src = (const char*)srcq;
		dest = (char*)destq;
	}

    while (n--)
        *dest++ = *src++;

    return s1;
}
