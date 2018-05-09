#pragma once

#include <stddef.h>
#include <stdint.h>


using addr_t = uint64_t;

void * operator new (size_t, void *p) noexcept;


namespace kutil {

/// Fill memory with the given value.
/// \arg p   The beginning of the memory area to fill
/// \arg v   The byte value to fill memory with
/// \arg n   The size in bytes of the memory area
/// \returns A pointer to the filled memory
void * memset(void *p, uint8_t v, size_t n);

/// Copy an area of memory to another
/// \dest    The memory to copy to
/// \src     The memory to copy from
/// \n       The number of bytes to copy
/// \returns A pointer to the destination memory
void * memcpy(void *dest, void *src, size_t n);

template <typename T>
inline T read_from(const void *p) { return *reinterpret_cast<const T *>(p); }

template <typename T>
inline T * offset_pointer(T *p, size_t offset)
{
	return reinterpret_cast<T *>(reinterpret_cast<addr_t>(p) + offset);
}

template <typename T>
inline T* mask_pointer(T *p, addr_t mask)
{
	return reinterpret_cast<T *>(reinterpret_cast<addr_t>(p) & ~mask);
}

} // namespace kutil
