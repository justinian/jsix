#include "kutil/memory.h"
#include "kutil/memory_manager.h"

namespace std {
	enum class __attribute__ ((__type_visibility("default"))) align_val_t : size_t { };
}

#ifdef __POPCORN__
void * operator new(size_t n, std::align_val_t) { return kutil::malloc(n); }
void * operator new (size_t n)					{ return kutil::malloc(n); }
void * operator new[] (size_t n)				{ return kutil::malloc(n); }
void operator delete (void *p) noexcept			{ return kutil::free(p); }
void operator delete[] (void *p) noexcept		{ return kutil::free(p); }
#endif

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
