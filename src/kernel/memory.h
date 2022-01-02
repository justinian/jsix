#pragma once

#include <stddef.h>
#include <stdint.h>


void * operator new (size_t, void *p) noexcept;

/// Allocate from the default allocator.
/// \arg size  The size in bytes requested
/// \returns   A pointer to the newly allocated memory,
///            or nullptr on error
void * kalloc(size_t size);

/// Free memory allocated by `kalloc`.
/// \arg p  Pointer that was returned from a `kalloc` call
void kfree(void *p);

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
    return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(p) + n);
}

/// Return a pointer with the given bits masked out
/// \arg p    The original pointer
/// \arg mask A bitmask of bits to clear from p
/// \returns  The masked pointer
template <typename T>
inline T* mask_pointer(T *p, uintptr_t mask)
{
    return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(p) & ~mask);
}

/// Do a simple byte-wise checksum of an area of memory.
/// \arg p    The start of the memory region
/// \arg len  The number of bytes in the region
/// \arg off  An optional offset into the region
uint8_t checksum(const void *p, size_t len, size_t off = 0);
