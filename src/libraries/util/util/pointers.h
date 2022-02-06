/// \file pointers.h
/// Helper functions and types for doing type-safe byte-wise pointer math.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace util {

/// Return a pointer offset from `input` by `offset` bytes.
/// \arg input  The original pointer
/// \arg offset Offset `input` by this many bytes
template <typename T>
inline T* offset_pointer(T* input, ptrdiff_t offset) {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(input) + offset);
}

/// Return a pointer with the given bits masked out
/// \arg input The original pointer
/// \arg mask  A bitmask of bits to clear from p
/// \returns   The masked pointer
template <typename T>
inline T* mask_pointer(T *input, uintptr_t mask) {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(input) & ~mask);
}

/// Read a value of type T from a location in memory
/// \arg p   The location in memory to read
/// \returns The value at the given location cast to T
template <typename T>
inline T read_from(const void *p) {
    return *reinterpret_cast<const T *>(p);
}

/// Iterator for an array of `const T` whose size is known at runtime
/// \tparam T  Type of the objects in the array, whose size might not be
///            what is returned by sizeof(T).
template <typename T>
class const_offset_iterator
{
public:
    /// Constructor.
    /// \arg t    Pointer to the first item in the array
    /// \arg off  Offset applied to reach successive items. Default is 0,
    ///           which creates an effectively constant iterator.
    const_offset_iterator(T const *t, size_t off=0) : m_t(t), m_off(off) {}

    const T * operator++() { m_t = offset_pointer(m_t, m_off); return m_t; }
    const T * operator++(int) { T* tmp = m_t; operator++(); return tmp; }

    bool operator==(T* p) const { return p == m_t; }
    bool operator!=(T* p) const { return p != m_t; }
    bool operator==(const_offset_iterator<T> &i) const { return i.m_t == m_t; }
    bool operator!=(const_offset_iterator<T> &i) const { return i.m_t != m_t; }

    const T& operator*() const { return *m_t; }
    operator const T& () const { return *m_t; }
    const T* operator->() const { return m_t; }

private:
    T const *m_t;
    size_t m_off;
};

/// iterator for an array of `const T` whose size is known at runtime
/// \tparam T  type of the objects in the array, whose size might not be
///            what is returned by sizeof(T).
template <typename T>
class offset_iterator
{
public:
    /// constructor.
    /// \arg t    pointer to the first item in the array
    /// \arg off  offset applied to reach successive items. default is 0,
    ///           which creates an effectively constant iterator.
    offset_iterator(T *t, size_t off=0) : m_t(t), m_off(off) {}

    T * operator++() { m_t = offset_pointer(m_t, m_off); return m_t; }
    T * operator++(int) { T* tmp = m_t; operator++(); return tmp; }

    bool operator==(T *p) const { return p == m_t; }
    bool operator!=(T *p) const { return p != m_t; }
    bool operator==(offset_iterator<T> &i) const { return i.m_t == m_t; }
    bool operator!=(offset_iterator<T> &i) const { return i.m_t != m_t; }

    T & operator*() const { return *m_t; }
    operator T & () const { return *m_t; }
    T * operator->() const { return m_t; }

private:
    T *m_t;
    size_t m_off;
};

} // namespace util
