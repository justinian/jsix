#include "kernel_memory.h"
#include "objects/process.h"
#include "objects/vm_area.h"

using memory::frame_size;

vm_area::vm_area(size_t size, vm_flags flags) :
	m_size(size),
	m_flags(flags),
	kobject(kobject::type::vma)
{
}

vm_area::~vm_area()
{
}

size_t
vm_area::resize(size_t size)
{
	return m_size;
}

j6_status_t
vm_area::add_to(vm_space *space, uintptr_t *base)
{
	if (!base || !space)
		return j6_err_invalid_arg;

	uintptr_t *prev = m_procs.find(space);
	if (prev) {
		*base = *prev;
		return j6_status_exists;
	}

	if (!*base)
		return j6_err_nyi;

	m_procs.insert(space, *base);
	for (auto &m : m_mappings)
		if (m.state == state::mapped)
			space->page_in(*base + m.offset, m.count, m.phys);

	return j6_status_ok;
}

j6_status_t
vm_area::remove_from(vm_space *space)
{
	uintptr_t *base = m_procs.find(space);
	if (space && base) {
		for (auto &m : m_mappings)
			if (m.state == state::mapped)
				space->page_out(*base + m.offset, m.count);
		m_procs.erase(space);
	}
	return j6_status_ok;
}

size_t
vm_area::overlaps(uintptr_t offset, size_t pages, size_t *count)
{
	size_t first = 0;
	size_t n = 0;

	uintptr_t end = offset + pages * frame_size;
	for (size_t i = 0; i < m_mappings.count(); ++i) {
		mapping &m = m_mappings[i];
		uintptr_t map_end = m.offset + m.count * frame_size;
		if (offset < map_end && end > m.offset) {
			if (!first) first = i;
			++n;
		} else if (n) {
			break;
		}
	}

	if (count) *count = n;
	return first;
}

bool
vm_area::commit(uintptr_t phys, uintptr_t offset, size_t count)
{
	return add(offset, count, state::mapped, phys);
}

bool
vm_area::uncommit(uintptr_t offset, size_t count)
{
	return remove(offset, count, state::reserved);
}

bool
vm_area::reserve(uintptr_t offset, size_t count)
{
	return add(offset, count, state::reserved, 0);
}

bool
vm_area::unreserve(uintptr_t offset, size_t count)
{
	return remove(offset, count, state::reserved);
}

vm_area::state
vm_area::get(uintptr_t offset, uintptr_t *phys)
{
	size_t n = 0;
	size_t o = overlaps(offset, 1, &n);
	if (n) {
		mapping &m = m_mappings[o];
		if (phys) *phys = m.phys;
		return m.state;
	}

	return state::none;
}

bool
vm_area::add(uintptr_t offset, size_t count, state desired, uintptr_t phys)
{
	const bool do_map = desired == state::mapped;

	size_t n = 0;
	size_t o = overlaps(offset, count, &n);
	if (!n) {
		// In the clear, map it
		size_t o = m_mappings.sorted_insert({
			.offset = offset,
			.count = count,
			.phys = phys,
			.state = desired});
		n = 1;

		if (do_map)
			map(offset, count, phys);
	} else if (desired == state::mapped) {
		// Mapping overlaps not allowed
		return false;
	}

	// Any overlaps with different states is not allowed
	for (size_t i = o; i < o+n; ++i)
		if (m_mappings[i].state != desired)
			return false;

	// Try to expand to abutting similar areas
	if (o > 0 &&
		m_mappings[o-1].state == desired &&
		m_mappings[o-1].end() == offset &&
		(!do_map || m_mappings[o-1].phys_end() == phys)) {
		--o;
		++n;
	}

	uintptr_t end = offset + count * frame_size;
	uintptr_t pend = offset + count * frame_size;
	if (o + n < m_mappings.count() &&
		m_mappings[o+n].state == desired &&
		end == m_mappings[o+n].offset &&
		(!do_map || m_mappings[o-1].phys == pend)) {
		++n;
	}

	// Use the first overlap block as our new block
	mapping &first = m_mappings[o];
	mapping &last = m_mappings[o + n -1];

	if (offset < first.offset)
		first.offset = offset;

	size_t diff =
		(end > last.end() ? end : last.end()) -
		first.offset;

	first.count = diff / frame_size;

	if (n > 1)
		m_mappings.remove_at(o+1, n-1);

	return true;
}


bool
vm_area::remove(uintptr_t offset, size_t count, state expected)
{
	size_t n = 0;
	size_t o = overlaps(offset, count, &n);
	if (!n) return true;

	// Any overlaps with different states is not allowed
	for (size_t i = o; i < o+n; ++i)
		if (m_mappings[i].state != expected)
			return false;

	mapping *first = &m_mappings[o];
	mapping *last = &m_mappings[o+n-1];

	uintptr_t end = offset + count * frame_size;

	size_t leading = offset - first->offset;
	size_t trailing = last->end() - end;

	// if were entirely contained in one, we need to split it
	if (leading && trailing && n == 1) {
		size_t i = m_mappings.sorted_insert({
			.offset = end,
			.count = trailing / frame_size,
			.state = first->state,
			});
		last = &m_mappings[i];
		trailing = 0;

		first->count -= last->count;
		if (first->state == state::mapped)
			last->phys = first->phys + first->count * frame_size;
	}

	if (leading) {
		size_t remove_pages = first->count; 
		first->count = leading / frame_size;
		remove_pages -= first->count;
		if (expected == state::mapped)
			unmap(first->end(), remove_pages);
	}

	if (trailing) {
		uintptr_t remove_off = last->offset;
		size_t remove_pages = last->count; 
		last->offset = end;
		last->count = trailing / frame_size;
		remove_pages -= last->count;
		if (expected == state::mapped) {
			unmap(remove_off, remove_pages);
			last->phys += remove_pages * frame_size;
		}
	}

	size_t delete_start = 0;
	size_t delete_count = 0;
	for (size_t i = o; i < o+n; ++i) {
		mapping &m = m_mappings[i];
		if (offset <= m.offset && end >= m.end()) {
			if (!delete_count) delete_start = i;
			++delete_count;
			if (expected == state::mapped)
				unmap(m.offset, m.count);
		}
	}

	if (delete_count)
		m_mappings.remove_at(delete_start, delete_count);

	return true;
}

void
vm_area::map(uintptr_t offset, size_t count, uintptr_t phys)
{
	for (auto &it : m_procs) {
		uintptr_t addr = it.val + offset;
		vm_space *space = it.key;
		space->page_in(addr, count, phys);
	}
}

void
vm_area::unmap(uintptr_t offset, size_t count)
{
	for (auto &it : m_procs) {
		uintptr_t addr = it.val + offset;
		vm_space *space = it.key;
		space->page_out(addr, count);
	}
}

