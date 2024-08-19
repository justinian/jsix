/// \file pointers.h
/// Helper functions and types for doing type-safe byte-wise pointer math.
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <util/basic_types.h>

namespace util {

/// Return a pointer offset from `input` by `offset` bytes.
/// \arg input  The original pointer
/// \arg offset Offset `input` by this many bytes
template <typename T>
inline T* offset_pointer(T* input, ptrdiff_t offset) {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(input) + offset);
}

/// Get the offset from ptr1 to ptr2.
/// \arg ptr1  The base pointer
/// \arg ptr2  The offset pointer
/// \returns   Offset from pt1 to ptr2
template <typename T, typename U>
inline ptrdiff_t get_offset(T* ptr1, U* ptr2) {
    return reinterpret_cast<const char*>(ptr2) - reinterpret_cast<const char*>(ptr1);
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


template <typename T>
class unique_ptr
{
public:
    unique_ptr() : m_value {nullptr} {}
    unique_ptr(T *p) : m_value {p} {}
    unique_ptr(unique_ptr &&o) { *this = util::move(o); }

    ~unique_ptr() { delete m_value; }

    unique_ptr & operator=(unique_ptr &&o) {
        if (o.m_value == m_value) return *this;
        delete m_value;

        m_value = o.m_value;
        o.m_value = nullptr;
        return *this;
    }

    unique_ptr(const unique_ptr &) = delete;
    unique_ptr & operator=(const unique_ptr &) = delete;

    T* get() { return m_value; }
    const T* get() const { return m_value; }
    operator bool() const { return m_value; }
    bool empty() const { return m_value = nullptr; }

    T* operator->() { return m_value; }
    const T operator->() const { return m_value; }

    T operator*() { return *m_value; }
    const T operator*() const { return *m_value; }

    operator T*() { return m_value; }
    operator const T*() const { return m_value; }

private:
    T *m_value = nullptr;
};

} // namespace util
