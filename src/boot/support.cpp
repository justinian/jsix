#include <stdint.h>

#include "error.h"

extern "C" {

/// Basic memcpy() implementation for clang. Clang requires freestanding code
/// implement memcpy(), as it may emit references to it. This basic memcpy is
/// not the most efficient, but will get linked if no other memcpy exists.
__attribute__ ((__weak__))
void *memcpy(void *dest, const void *src, size_t n)
{
	uint8_t *cdest = reinterpret_cast<uint8_t*>(dest);
	const uint8_t *csrc = reinterpret_cast<const uint8_t*>(src);
	for (size_t i = 0; i < n; ++i)
		cdest[i] = csrc[i];
	return dest;
}

/// Basic memset() implementation for clang. Clang requires freestanding code
/// implement memset(), as it may emit references to it. This basic memset is
/// not the most efficient, but will get linked if no other memcpy exists.
__attribute__ ((__weak__))
void *memset(void *dest, int c, size_t n)
{
	uint8_t *cdest = reinterpret_cast<uint8_t*>(dest);
	for (size_t i = 0; i < n; ++i)
		cdest[i] = static_cast<uint8_t>(c);
	return dest;
}

int _purecall()
{
	::boot::error::raise(uefi::status::unsupported, L"Pure virtual call");
}

} // extern "C"

void operator delete (void *) {}
