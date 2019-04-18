#include <algorithm>
#include "kutil/logger.h"
#include "kutil/vm_space.h"

namespace kutil {

using node_type = kutil::avl_node<vm_range>;


vm_space::vm_space(uintptr_t start, size_t size, allocator &alloc) :
	m_alloc(alloc)
{
	node_type *node = m_alloc.pop();
	node->address = start;
	node->size = size;
	node->state = vm_state::none;
	m_ranges.insert(node);

	log::info(logs::vmem, "Creating address space from %016llx-%016llx",
			start, start+size);
}

inline static bool
overlaps(node_type *node, uintptr_t start, size_t size)
{
	return start <= node->end() &&
		   (start + size) >= node->address;
}

static node_type *
find_overlapping(node_type *from, uintptr_t start, size_t size)
{
	while (from) {
		if (overlaps(from, start, size))
			return from;

		from = start < from->address ?
			from->left() :
			from->right();
	}

	return nullptr;
}

node_type *
vm_space::split_out(node_type *node, uintptr_t start, size_t size, vm_state state)
{
	// No cross-boundary splits allowed for now
	bool contained = 
		start >= node->address &&
		start+size <= node->end();

	kassert(contained,
		"Tried to split an address range across existing boundaries");
	if (!contained)
		return nullptr;

	vm_state old_state = node->state;
	if (state == old_state)
		return node;

	log::debug(logs::vmem, "Splitting out region %016llx-%016llx[%d] from %016llx-%016llx[%d]",
			start, start+size, state, node->address, node->end(), old_state);

	if (node->address < start) {
		// Split off rest into new node
		size_t leading = start - node->address;

		node_type *next = m_alloc.pop();
		next->state = state;
		next->address = start;
		next->size = node->size - leading;
		node->size = leading;

		m_ranges.insert(next);
		node = next;
	} else {
		// TODO: check if this can merge with prev node
	}

	if (node->end() > start + size) {
		// Split off remaining into new node
		size_t trailing =  node->size - size;
		node->size -= trailing;

		node_type *next = m_alloc.pop();
		next->state = old_state;
		next->address = node->end();
		next->size = trailing;

		m_ranges.insert(next);
	} else {
		// TODO: check if this can merge with next node
	}

	return node;
}

uintptr_t
vm_space::reserve(uintptr_t start, size_t size)
{
	log::debug(logs::vmem, "Reserving region %016llx-%016llx", start, start+size);

	node_type *node = find_overlapping(m_ranges.root(), start, size);
	if (!node || node->state != vm_state::none)
		return 0;

	node = split_out(node, start, size, vm_state::reserved);
	return node ? start : 0;
}

void
vm_space::unreserve(uintptr_t start, size_t size)
{
}

uintptr_t
vm_space::commit(uintptr_t start, size_t size)
{
	return 0;
}

void
vm_space::uncommit(uintptr_t start, size_t size)
{
}


vm_state
vm_space::get(uintptr_t addr)
{
	node_type *node = find_overlapping(m_ranges.root(), addr, 0);
	return node ? node->state : vm_state::unknown;
}

} // namespace kutil
