#pragma once
/// \file counted.h
/// Definition of the `counted` template class

#include <util/pointers.h>

namespace util {

/// A pointer and an associated count. Memory pointed to is not managed.
/// Depending on the usage, the count may be size or number of elements.
/// Helper methods provide the ability to treat the pointer like an array.
template <typename T>
struct counted
{
    T *pointer;
    size_t count;

    /// Index this object as an array of type T
    inline T & operator [] (int i) { return pointer[i]; }

    /// Index this object as a const array of type T
    inline const T & operator [] (int i) const { return pointer[i]; }

    using iterator = offset_iterator<T>;
    using const_iterator = const_offset_iterator<T>;

    /// Return an iterator to the beginning of the array
    inline iterator begin() { return iterator(pointer, sizeof(T)); }

    /// Return an iterator to the end of the array
    inline iterator end() { return offset_pointer<T>(pointer, sizeof(T)*count); }

    /// Return an iterator to the beginning of the array
    inline const_iterator begin() const { return const_iterator(pointer, sizeof(T)); }

    /// Return an iterator to the end of the array
    inline const_iterator end() const { return offset_pointer<const T>(pointer, sizeof(T)*count); }

    /// Advance the pointer by N items
    inline counted<T> & operator+=(unsigned i) {
        if (i > count) i = count;
        pointer += i;
        count -= i;
        return *this;
    }

    /// Get a constant buffer from a const pointer
    static const counted<T> from_const(const T *p, size_t count) {
        return { const_cast<T*>(p), count };
    }
};

/// Specialize for `void` which cannot be indexed or iterated
template <>
struct counted<void>
{
    void *pointer;
    size_t count;

    /// Advance the pointer by N bytes
    inline counted<void> & operator+=(unsigned i) {
        if (i > count) i = count;
        pointer = offset_pointer(pointer, i);
        count -= i;
        return *this;
    }

    /// Get a constant buffer from a const pointer
    static const counted<void> from_const(const void *p, size_t count) {
        return { const_cast<void*>(p), count };
    }
};

using buffer = counted<void>;

template <typename T>
const T * read(buffer &b) {
    if (b.count < sizeof(T))
        return nullptr;

    const T *p = reinterpret_cast<const T*>(b.pointer);
    b.pointer = offset_pointer(b.pointer, sizeof(T));
    b.count -= sizeof(T);
    return p;
}

} // namespace util
