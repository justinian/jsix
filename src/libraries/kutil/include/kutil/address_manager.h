#pragma once
/// \file address_manager.h
/// The virtual memory address space manager

#include <stdint.h>
#include "kutil/linked_list.h"
#include "kutil/slab_allocator.h"

namespace kutil {

class address_manager
{
public:
	/// Constructor.
	/// \arg start   Initial address in the managed range
	/// \arg length  Size of the managed range, in bytes
	address_manager(uintptr_t start, size_t length);

	/// Allocate address space from the managed area.
	/// \arg length  The amount of memory to allocate, in bytes
	/// \returns     The address of the start of the allocated area, or 0 on
	///              failure
	uintptr_t allocate(size_t length);

	/// Mark a region as allocated.
	/// \arg start   The start of the region
	/// \arg length  The size of the region, in bytes
	/// \returns     The address of the start of the allocated area, or 0 on
	///              failure. This may be less than `start`.
	uintptr_t mark(uintptr_t start, size_t length);

	/// Free a previous allocation.
	/// \arg p  An address previously retuned by allocate()
	void free(uintptr_t p);

private:
	struct region
	{
		inline int compare(const region *o) const {
			return address > o->address ? 1 : \
				address < o->address ? -1 : 0;
		}

		inline uintptr_t end() const { return address + (1ull << size); }
		inline uintptr_t half() const { return address + (1ull << (size - 1)); }
		inline uintptr_t buddy() const { return address ^ (1ull << size); }
		inline bool elder() const { return address < buddy(); }

		uint16_t size;
		uintptr_t address;
	};

	using region_node = list_node<region>;
	using region_list = linked_list<region>;

	/// Split a region of the given size into two smaller regions, returning
	/// the new latter half
	region_node * split(region_node *reg);

	/// Find a node with the given address
	region_node * find(uintptr_t p, bool used = true);

	/// Helper to get the buddy for a node, if it's adjacent
	region_node * get_buddy(region_node *r);

	linked_list<region> & used_bucket(unsigned size) { return m_used[size - size_min]; }
	linked_list<region> & free_bucket(unsigned size) { return m_free[size - size_min]; }

	static const unsigned size_min = 16;  // Min allocation: 64KiB
	static const unsigned size_max = 36;  // Max allocation: 64GiB
	static const unsigned buckets = (size_max - size_min);

	region_list m_free[buckets];
	region_list m_used[buckets];
	slab_allocator<region> m_alloc;
};

} //namespace kutil
