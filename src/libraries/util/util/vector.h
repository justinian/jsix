#pragma once
/// \file vector.h
/// Definition of a simple dynamic vector collection for use in kernel space

#include <assert.h>
#include <new>
#include <string.h>
#include <utility>

#include <util/allocator.h>
#include <util/util.h>

namespace util {

/// A dynamic array.
template <typename T, typename S = uint32_t, typename alloc = default_allocator>
class vector
{
public:
    using count_t = S;
    static constexpr count_t min_capacity = 4;
    static constexpr count_t cap_mask = static_cast<S>(-1) >> 1;

    /// Default constructor. Creates an empty vector with no capacity.
    vector() :
        m_size(0),
        m_capacity(0),
        m_elements(nullptr)
    {}

    /// Constructor. Creates an empty array with capacity.
    /// \arg capacity  Initial capacity to allocate
    vector(count_t capacity) :
        m_size(0),
        m_capacity(0),
        m_elements(nullptr)
    {
        set_capacity(capacity);
    }

    /// Copy constructor. Allocates a copy of the other's array.
    vector(const vector& other) :
        m_size(0),
        m_capacity(0),
        m_elements(nullptr)
    {
        set_capacity(other.m_capacity);
        memcpy(m_elements, other.m_elements, other.m_size * sizeof(T));
        m_size = other.m_size;
    }

    /// Move constructor. Takes ownership of the other's array.
    vector(vector &&other) :
        m_size(other.m_size),
        m_capacity(other.m_capacity),
        m_elements(other.m_elements)
    {
        other.m_size = 0;
        other.m_capacity = 0;
        other.m_elements = nullptr;
    }

    /// Static array constructor. Starts the vector off with the given
    /// static storage.
    vector(T *data, count_t size, count_t capacity) :
        m_size(size),
        m_capacity(capacity | ~cap_mask),
        m_elements(&data[0])
    {
    }

    /// Destructor. Destroys any remaining items in the array.
    ~vector()
    {
        while (m_size) remove();

        bool was_static = m_capacity & ~cap_mask;
        if (!was_static)
            alloc::free(m_elements);
    }

    /// Get the size of the array.
    inline count_t count() const { return m_size; }

    /// Check if the array is empty
    inline bool empty() const { return m_size == 0; }

    /// Get the capacity of the array. This is the amount of space
    /// actually allocated.
    inline count_t capacity() const { return m_capacity & cap_mask; }

    /// Access an element in the array.
    inline T & operator[] (count_t i) { return m_elements[i]; }

    /// Access an element in the array.
    inline const T & operator[] (count_t i) const { return m_elements[i]; }

    /// Get a pointer to the beginning for iteration.
    T * begin() { return m_elements; }

    /// Get a pointer to the beginning for iteration.
    const T * begin() const { return m_elements; }

    /// Get a pointer to the end for iteration.
    T * end() { return m_elements + m_size; }

    /// Get a pointer to the end for iteration.
    const T * end() const { return m_elements + m_size; }

    /// Add an item onto the array by copying it.
    /// \arg item  The item to add
    /// \returns   A reference to the added item
    T & append(const T& item)
    {
        ensure_capacity(m_size + 1);
        m_elements[m_size] = item;
        return m_elements[m_size++];
    }

    /// Construct an item in place onto the end of the array.
    /// \returns   A reference to the added item
    template <typename... Args>
    T & emplace(Args&&... args)
    {
        ensure_capacity(m_size + 1);
        new (&m_elements[m_size]) T(std::forward<Args>(args)...);
        return m_elements[m_size++];
    }

    /// Insert an item into the array at the given index
    void insert(count_t i, const T& item)
    {
        if (i >= count()) {
            append(item);
            return;
        }

        ensure_capacity(m_size + 1);
        for (count_t j = m_size; j > i; --j)
            m_elements[j] = m_elements[j-1];
        m_size += 1;

        m_elements[i] = item;
    }

    /// Insert an item into the list in a sorted position. Depends on T
    /// having a method `int compare(const T &other)`.
    /// \returns index of the new item
    count_t sorted_insert(const T& item)
    {
        count_t start = 0;
        count_t end = m_size;
        while (end > start) {
            count_t m = start + (end - start) / 2;
            int c = item.compare(m_elements[m]);
            if (c < 0) end = m;
            else start = m + 1;
        }

        insert(start, item);
        return start;
    }

    /// Remove an item from the end of the array.
    void remove()
    {
        assert(m_size && "Called remove() on an empty array");

        m_size -= 1;
        m_elements[m_size].~T();
    }

    /// Remove an item from the front of the array, preserving order.
    void remove_front()
    {
        assert(m_size && "Called remove_front() on an empty array");
        remove_at(0);
    }

    /// Remove an item from the array.
    void remove(const T &item)
    {
        assert(m_size && "Called remove() on an empty array");
        for (count_t i = 0; i < m_size; ++i) {
            if (m_elements[i] == item) {
                remove_at(i);
                break;
            }
        }
    }

    /// Remove n items starting at the given index from the array,
    /// order-preserving.
    void remove_at(count_t i, count_t n = 1)
    {
        for (count_t j = i; j < i + n; ++j) {
            if (j >= m_size) return;
            m_elements[j].~T();
        }

        for (; i < m_size - n; ++i)
            m_elements[i] = m_elements[i+n];
        m_size -= n;
    }

    /// Remove the first occurance of an item from the array, not
    /// order-preserving. Does nothing if the item is not in the array.
    void remove_swap(const T &item)
    {
        for (count_t i = 0; i < m_size; ++i) {
            if (m_elements[i] == item) {
                remove_swap_at(i);
                break;
            }
        }
    }

    /// Remove the item at the given index from the array, not
    /// order-preserving.
    void remove_swap_at(count_t i)
    {
        if (i >= count()) return;

        m_elements[i].~T();
        if (i < m_size - 1)
            m_elements[i] = m_elements[m_size - 1];
        m_size -= 1;
    }

    /// Remove an item from the end of the array and return it.
    T pop()
    {
        assert(m_size && "Called pop() on an empty array");

        T temp = m_elements[m_size - 1];
        remove();
        return temp;
    }

    /// Remove an item from the beginning of the array and return it.
    T pop_front()
    {
        assert(m_size && "Called pop_front() on an empty array");

        T temp = m_elements[0];
        remove_front();
        return temp;
    }

    /// Set the size of the array. Any new items are default constructed.
    /// Any items past the end are deleted. The array is realloced if needed.
    /// \arg size  The new size
    void set_size(count_t size)
    {
        ensure_capacity(size);
        for (count_t i = size; i < m_size; ++i)
            m_elements[i].~T();
        for (count_t i = m_size; i < size; ++i)
            new (&m_elements[i]) T;
        m_size = size;
    }

    /// Ensure the array will fit an item.
    /// \arg size  Size of the array
    void ensure_capacity(count_t size)
    {
        if (capacity() >= size) return;
        count_t capacity = (1 << log2(size));
        if (capacity < min_capacity)
            capacity = min_capacity;
        set_capacity(capacity);
    }

    /// Reallocate the array. Copy over any old elements that will
    /// fit into the new array. The rest are destroyed.
    /// \arg capacity  Number of elements to allocate
    void set_capacity(count_t capacity)
    {
        while (capacity < m_size) remove();

        bool was_static = m_capacity & ~cap_mask;
        if (was_static) {
            T *new_array = reinterpret_cast<T*>(alloc::allocate(capacity * sizeof(T)));
            memcpy(new_array, m_elements, m_size * sizeof(T));
            m_elements = new_array;
        } else {
            m_elements = reinterpret_cast<T*>(
                    alloc::realloc(m_elements, m_capacity * sizeof(T), capacity * sizeof(T)));
        }

        m_capacity = capacity;
    }

private:
    count_t m_size;
    count_t m_capacity;
    T *m_elements;
};

} // namespace util
