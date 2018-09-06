#pragma once

#include <stddef.h>
#include <stdint.h>


using addr_t = uint64_t;

void * operator new (size_t, void *p) noexcept;


namespace kutil {

/// Allocate memory. Note: this needs to be implemented
/// by the kernel, or other program using this library.
/// \arg n   The number of bytes to allocate
/// \returns The allocated memory
void * malloc(size_t n);

/// Free memory allocated by malloc(). Note: this needs
/// to be implemented by the kernel, or other program
/// using this library.
/// \arg p   A pointer previously returned by malloc()
void free(void *p);

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
void * memcpy(void *dest, const void *src, size_t n);

/// Read a value of type T from a location in memory
/// \arg p   The location in memory to read
/// \returns The value at the given location cast to T
template <typename T>
inline T read_from(const void *p)
{
	return *reinterpret_cast<const T *>(p);
}

/// Get a pointer that's offset from another pointer
/// \arg p   The base pointer
/// \arg n   The offset in bytes
/// \returns The offset pointer
template <typename T>
inline T * offset_pointer(T *p, ptrdiff_t n)
{
	return reinterpret_cast<T *>(reinterpret_cast<addr_t>(p) + n);
}

/// Return a pointer with the given bits masked out
/// \arg p    The original pointer
/// \arg mask A bitmask of bits to clear from p
/// \returns  The masked pointer
template <typename T>
inline T* mask_pointer(T *p, addr_t mask)
{
	return reinterpret_cast<T *>(reinterpret_cast<addr_t>(p) & ~mask);
}

/// Do a simple byte-wise checksum of an area of memory.
/// \arg p    The start of the memory region
/// \arg len  The number of bytes in the region
/// \arg off  An optional offset into the region
uint8_t checksum(const void *p, size_t len, size_t off = 0);

} // namespace kutil
