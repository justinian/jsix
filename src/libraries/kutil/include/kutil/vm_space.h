#pragma once
/// \file vm_range.h
/// Structure for tracking a range of virtual memory addresses

#include <stdint.h>
#include "kutil/allocator.h"
#include "kutil/avl_tree.h"

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

/// Tracks a region of virtual memory address space
class vm_space
{
public:
	/// Default constructor. Define an empty range.
	vm_space();

	/// Constructor. Define a range of managed VM space.
	/// \arg start       Starting address of the managed space
	/// \arg size        Size of the managed space, in bytes
	vm_space(uintptr_t start, size_t size);

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

	tree_type m_ranges;
};

} // namespace kutil
