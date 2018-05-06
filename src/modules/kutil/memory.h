#pragma once

#include <stddef.h>
#include <stdint.h>


using addr_t = uint64_t;

namespace kutil {

void * memset(void *p, uint8_t v, size_t n);

template <typename T>
inline T read_from(const void *p) { return *reinterpret_cast<const T *>(p); }

template <typename T>
inline T * offset_pointer(T *p, size_t offset)
{
	return reinterpret_cast<T *>(reinterpret_cast<addr_t>(p) + offset);
}

} // namespace kutil
