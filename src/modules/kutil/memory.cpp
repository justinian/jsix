#include "memory.h"

void * operator new (size_t, void *p) noexcept	{ return p; }
void * operator new (size_t n)					{ return kutil::malloc(n); }
void * operator new[] (size_t n)				{ return kutil::malloc(n); }
void operator delete (void *p) noexcept			{ return kutil::free(p); }
void operator delete[] (void *p) noexcept		{ return kutil::free(p); }

namespace kutil {

void *
memset(void *s, uint8_t v, size_t n)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(s);
	for (size_t i = 0; i < n; ++i) p[i] = v;
	return s;
}

void *
memcpy(void *dest, void *src, size_t n)
{
	uint8_t *s = reinterpret_cast<uint8_t *>(src);
	uint8_t *d = reinterpret_cast<uint8_t *>(dest);
	for (size_t i = 0; i < n; ++i) d[i] = s[i];
	return d;
}

} // namespace kutil
