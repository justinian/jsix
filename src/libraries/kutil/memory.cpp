#include "kutil/memory.h"
#include "kutil/memory_manager.h"
#include "kutil/type_macros.h"

__weak void * operator new (size_t, void *p) noexcept	{ return p; }
__weak void * operator new (size_t n)					{ return kutil::malloc(n); }
__weak void * operator new[] (size_t n)					{ return kutil::malloc(n); }
__weak void operator delete (void *p) noexcept			{ return kutil::free(p); }
__weak void operator delete[] (void *p) noexcept		{ return kutil::free(p); }

namespace kutil {

namespace setup {

static memory_manager *heap_memory_manager;

void
set_heap(memory_manager *mm)
{
	setup::heap_memory_manager = mm;
}

} // namespace kutil::setup


void *
malloc(size_t n)
{
	return setup::heap_memory_manager->allocate(n);
}

void
free(void *p)
{
	setup::heap_memory_manager->free(p);
}


void *
memset(void *s, uint8_t v, size_t n)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(s);
	for (size_t i = 0; i < n; ++i) p[i] = v;
	return s;
}

void *
memcpy(void *dest, const void *src, size_t n)
{
	const uint8_t *s = reinterpret_cast<const uint8_t *>(src);
	uint8_t *d = reinterpret_cast<uint8_t *>(dest);
	for (size_t i = 0; i < n; ++i) d[i] = s[i];
	return d;
}

uint8_t
checksum(const void *p, size_t len, size_t off)
{
	uint8_t sum = 0;
	const uint8_t *c = reinterpret_cast<const uint8_t *>(p);
	for (int i = off; i < len; ++i) sum += c[i];
	return sum;
}

} // namespace kutil
