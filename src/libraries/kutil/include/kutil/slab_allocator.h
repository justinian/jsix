#pragma once
/// \file slab_allocator.h
/// A slab allocator and related definitions
#include "kutil/allocator.h"
#include "kutil/assert.h"
#include "kutil/linked_list.h"
#include "kutil/memory.h"

namespace kutil {


/// A slab allocator for small structures kept in a linked list
template <typename T, size_t N = memory::frame_size>
class slab_allocator :
	public linked_list<T>,
	public allocator
{
public:
	using item_type = list_node<T>;

	/// Default constructor.
	/// \arg alloc      The allocator to use to allocate chunks. Defaults to malloc().
	slab_allocator(allocator &alloc) :
		m_alloc(alloc) {}

	/// Allocator interface implementation
	virtual void * allocate(size_t size) override
	{
		kassert(size == sizeof(T), "Asked slab allocator for wrong size");
		if (size != sizeof(T)) return 0;
		return pop();
	}

	/// Allocator interface implementation
	virtual void free(void *p) override
	{
		push(static_cast<item_type*>(p));
	}

	/// Get an item from the cache. May allocate a new chunk if the cache is empty.
	/// \returns  An allocated element
	inline item_type * pop()
	{
		if (this->empty()) this->allocate_chunk();
		kassert(!this->empty(), "Slab allocator is empty after allocate()");
		item_type *item = this->pop_front();
		kutil::memset(item, 0, sizeof(item_type));
		return item;
	}

	/// Return an item to the cache.
	/// \arg item  A previously allocated element
	inline void push(item_type *item)
	{
		this->push_front(item);
	}

private:
	void allocate_chunk()
	{
		constexpr unsigned count = N / sizeof(item_type);

		void *memory = m_alloc.allocate(N);
		item_type *items = reinterpret_cast<item_type *>(memory); 
		for (size_t i = 0; i < count; ++i)
			this->push_back(&items[i]);
	}

	allocator& m_alloc;
};

} // namespace kutil
