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
    T *pointer = nullptr;
    size_t count = 0;

    /// Index this object as an array of type T
    inline T & operator [] (int i) { return pointer[i]; }

    /// Index this object as a const array of type T
    inline const T & operator [] (int i) const { return pointer[i]; }

    operator bool() const { return pointer != nullptr; }
    bool operator!() const { return pointer == nullptr; }

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

    /// Return a counted<T> advanced by N items
    inline counted<T> operator+(size_t i) {
        counted<T> other = *this;
        other += i;
        return other;
    }

    /// Return a const counted<T> advanced by N items
    inline const counted<T> operator+(size_t i) const {
        counted<T> other = *this;
        other += i;
        return other;
    }

    /// Advance the pointer by N items
    inline counted<T> & operator+=(size_t i) {
        if (i > count) i = count;
        pointer += i;
        count -= i;
        return *this;
    }
};

/// Specialize for `const void` which cannot be indexed or iterated
template <>
struct counted<const void>
{
    const void *pointer = nullptr;
    size_t count = 0;

    template <typename T>
    static inline counted<const void> from(T p, size_t c) {
        return {reinterpret_cast<const void*>(p), c};
    }

    operator bool() const { return pointer != nullptr; }
    bool operator!() const { return pointer == nullptr; }

    /// Return a counted<T> advanced by N items
    inline counted<const void> operator+(size_t i) {
        counted<const void> other = *this;
        other += i;
        return other;
    }

    /// Return a const counted<T> advanced by N items
    inline const counted<const void> operator+(size_t i) const {
        counted<const void> other = *this;
        other += i;
        return other;
    }

    /// Advance the pointer by N bytes
    inline counted<const void> & operator+=(unsigned i) {
        if (i > count) i = count;
        pointer = offset_pointer(pointer, i);
        count -= i;
        return *this;
    }
};

/// Specialize for `void` which cannot be indexed or iterated
template <>
struct counted<void>
{
    void *pointer = nullptr;
    size_t count = 0;

    template <typename T>
    static inline counted<void> from(T p, size_t c) {
        return {reinterpret_cast<void*>(p), c};
    }

    operator counted<const void>() const { return {pointer, count}; }

    operator bool() const { return pointer != nullptr; }
    bool operator!() const { return pointer == nullptr; }

    /// Return a counted<T> advanced by N items
    inline counted<void> operator+(size_t i) {
        counted<void> other = *this;
        other += i;
        return other;
    }

    /// Return a const counted<T> advanced by N items
    inline const counted<void> operator+(size_t i) const {
        counted<void> other = *this;
        other += i;
        return other;
    }

    /// Advance the pointer by N bytes
    inline counted<void> & operator+=(unsigned i) {
        if (i > count) i = count;
        pointer = offset_pointer(pointer, i);
        count -= i;
        return *this;
    }
};

using buffer = counted<void>;
using const_buffer = counted<const void>;

template <typename T>
T * read(buffer &b) {
    T *p = reinterpret_cast<T*>(b.pointer);
    b += sizeof(T);
    return p;
}

template <typename T>
const T * read(const_buffer &b) {
    const T *p = reinterpret_cast<const T*>(b.pointer);
    b += sizeof(T);
    return p;
}

} // namespace util
