#pragma once
/// \file vector.h
/// Definition of a simple dynamic vector collection for use in kernel space

#include <algorithm>
#include <utility>
#include "kutil/memory.h"

namespace kutil {

/// A dynamic array.
template <typename T>
class vector
{
public:
	/// Default constructor. Creates an empty vector with no capacity.
	vector() :
		m_size(0),
		m_capacity(0),
		m_elements(nullptr)
	{}

	/// Constructor. Creates an empty array with capacity.
	/// \arg capacity  Initial capacity to allocate
	vector(size_t capacity) :
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
		kutil::memcpy(m_elements, other.m_elements, other.m_size * sizeof(T));
		m_size = other.m_size;
	}

	/// Move constructor. Takes ownership of the other's array.
	vector(vector&& other) :
		m_size(other.m_size),
		m_capacity(other.m_capacity),
		m_elements(other.m_elements)
	{
		other.m_size = 0;
		other.m_capacity = 0;
		other.m_elements = nullptr;
	}

	/// Destructor. Destroys any remaining items in the array.
	~vector()
	{
		while (m_size) remove();
		kfree(m_elements);
	}

	/// Get the size of the array.
	/// \returns  The number of elements in the array
	inline size_t count() const { return m_size; }

	/// Access an element in the array.
	inline T & operator[] (size_t i) { return m_elements[i]; }

	/// Access an element in the array.
	inline const T & operator[] (size_t i) const { return m_elements[i]; }

	/// Get a pointer to the beginning for iteration.
	/// \returns  A pointer to the beginning of the array
	T * begin() { return m_elements; }

	/// Get a pointer to the beginning for iteration.
	/// \returns  A pointer to the beginning of the array
	const T * begin() const { return m_elements; }

	/// Get a pointer to the end for iteration.
	/// \returns  A pointer to the end of the array
	T * end() { return m_elements + m_size; }

	/// Get a pointer to the end for iteration.
	/// \returns  A pointer to the end of the array
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

	/// Remove an item from the end of the array.
	void remove()
	{
		m_size -= 1;
		m_elements[m_size].~T();
	}

	/// Remove an item from the end of the array and return it.
	T pop()
	{
		T temp = m_elements[m_size - 1];
		remove();
		return temp;
	}

	/// Set the size of the array. Any new items are default constructed.
	/// Any items past the end are deleted. The array is realloced if needed.
	/// \arg size  The new size
	void set_size(size_t size)
	{
		ensure_capacity(size);
		for (size_t i = size; i < m_size; ++i)
			m_elements[i].~T();
		for (size_t i = m_size; i < size; ++i)
			new (&m_elements[i]) T;
		m_size = size;
	}

	/// Ensure the array will fit an item.
	/// \arg size  Size of the array
	void ensure_capacity(size_t size)
	{
		if (m_capacity >= size) return;

		size_t capacity = m_capacity;
		while (capacity < size) {
			if (capacity == 0) capacity = 4;
			else capacity *= 2;
		}
		set_capacity(capacity);
	}

	/// Reallocate the array. Copy over any old elements that will
	/// fit into the new array. The rest are destroyed.
	/// \arg capacity  Number of elements to allocate
	void set_capacity(size_t capacity)
	{
		T *new_array = reinterpret_cast<T*>(kalloc(capacity * sizeof(T)));
		size_t size = std::min(capacity, m_size);

		kutil::memcpy(new_array, m_elements, size * sizeof(T));

		while (size < m_size) remove();
		m_size = size;
		m_capacity = capacity;

		kfree(m_elements);
		m_elements = new_array;
	}

private:
	size_t m_size;
	size_t m_capacity;
	T *m_elements;
};

} // namespace kutil
