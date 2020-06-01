#include <algorithm>
#include "kutil/logger.h"
#include "kutil/vector.h"
#include "kutil/vm_space.h"

namespace kutil {

using node_type = kutil::avl_node<vm_range>;
using node_vec = kutil::vector<node_type*>;

DEFINE_SLAB_ALLOCATOR(node_type, 1);

vm_space::vm_space(uintptr_t start, size_t size)
{
	node_type *node = new node_type;
	node->address = start;
	node->size = size;
	node->state = vm_state::none;
	m_ranges.insert(node);

	log::info(logs::vmem, "Creating address space from %016llx-%016llx",
			start, start+size);
}

vm_space::vm_space()
{
}

inline static bool
overlaps(node_type *node, uintptr_t start, size_t size)
{
	return start < node->end() &&
		   (start + size) > node->address;
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

	node->state = state;

	log::debug(logs::vmem, "Splitting out region %016llx-%016llx[%d] from %016llx-%016llx[%d]",
			start, start+size, state, node->address, node->end(), old_state);

	bool do_consolidate = false;
	if (node->address < start) {
		// Split off rest into new node
		size_t leading = start - node->address;

		node_type *next = new node_type;
		next->state = state;
		next->address = start;
		next->size = node->size - leading;

		node->size = leading;
		node->state = old_state;

		log::debug(logs::vmem,
			"   leading region %016llx-%016llx[%d]",
			node->address, node->address + node->size, node->state);

		m_ranges.insert(next);
		node = next;
	} else {
		do_consolidate = true;
	}

	if (node->end() > start + size) {
		// Split off remaining into new node
		size_t trailing =  node->size - size;
		node->size -= trailing;

		node_type *next = new node_type;
		next->state = old_state;
		next->address = node->end();
		next->size = trailing;

		log::debug(logs::vmem,
			"   tailing region %016llx-%016llx[%d]",
			next->address, next->address + next->size, next->state);

		m_ranges.insert(next);
	} else {
		do_consolidate = true;
	}

	if (do_consolidate)
		node = consolidate(node);

	return node;
}

inline void gather(node_type *node, node_vec &vec)
{
	if (node) {
		gather(node->left(), vec);
		vec.append(node);
		gather(node->right(), vec);
	}
}

node_type *
vm_space::consolidate(node_type *needle)
{
	node_vec nodes(m_ranges.count());
	gather(m_ranges.root(), nodes);

	node_type *prev = nullptr;
	for (auto *node : nodes) {
		log::debug(logs::vmem,
			"* Existing region %016llx-%016llx[%d]",
			node->address, node->address + node->size, node->state);

		if (prev && node->address == prev->end() && node->state == prev->state) {
			log::debug(logs::vmem,
				"Joining regions %016llx-%016llx[%d] %016llx-%016llx[%d]",
				prev->address, prev->address + prev->size, prev->state,
				node->address, node->address + node->size, node->state);

			prev->size += node->size;
			if (needle == node)
				needle = prev;
			m_ranges.remove(node);
		} else {
			prev = node;
		}
	}

	return needle;
}

uintptr_t
vm_space::reserve(uintptr_t start, size_t size)
{
	log::debug(logs::vmem, "Reserving region %016llx-%016llx", start, start+size);

	node_type *node = find_overlapping(m_ranges.root(), start, size);
	if (!node) {
		log::debug(logs::vmem, "  found no match");
		return 0;
	} else if (node->state != vm_state::none) {
		log::debug(logs::vmem, "  found wrong state %016llx-%016llx[%d]",
			node->address, node->address + node->size, node->state);
		return 0;
	}

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
