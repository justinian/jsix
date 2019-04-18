#pragma once
/// \file vm_range.h
/// Structure for tracking a range of virtual memory addresses

#include <stdint.h>
#include "kutil/allocator.h"
#include "kutil/avl_tree.h"
#include "kutil/slab_allocator.h"

namespace kutil {

enum class vm_state : uint8_t {
	unknown,
	none,
	reserved,
	committed,
	mapped
};

struct vm_range
{
	uintptr_t address;
	size_t size;
	vm_state state;

	inline uintptr_t end() const { return address + size; }
	inline int compare(const vm_range *other) const {
		return other->address - address;
	}
};

class vm_space
{
public:
	vm_space(uintptr_t start, size_t size, kutil::allocator &alloc);

	/// Reserve a section of address space.
	/// \arg start  Starting address of reservaion, or 0 for any address
	/// \arg size   Size of reservation in bytes
	/// \returns  The address of the reservation, or 0 on failure
	uintptr_t reserve(uintptr_t start, size_t size);

	/// Unreserve (and uncommit, if committed) a section of address space.
	/// \arg start  Starting address of reservaion
	/// \arg size   Size of reservation in bytes
	void unreserve(uintptr_t start, size_t size);

	/// Mark a section of address space as committed.
	/// \arg start  Starting address of reservaion
	/// \arg size   Size of reservation in bytes
	/// \returns  The address of the reservation, or 0 on failure
	uintptr_t commit(uintptr_t start, size_t size);

	/// Mark a section of address space as uncommitted, but still reserved.
	/// \arg start  Starting address of reservaion
	/// \arg size   Size of reservation in bytes
	void uncommit(uintptr_t start, size_t size);

	/// Check the state of the given address.
	/// \arg addr  The address to check
	/// \returns   The state of the memory if known, or 'unknown'
	vm_state get(uintptr_t addr);

private:
	using node_type = kutil::avl_node<vm_range>;
	using tree_type = kutil::avl_tree<vm_range>;

	node_type * split_out(node_type* node, uintptr_t start, size_t size, vm_state state);
	node_type * consolidate(node_type* needle);

	slab_allocator<node_type> m_slab;
	allocator &m_alloc;
	tree_type m_ranges;
};

} // namespace kutil
