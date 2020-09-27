#include "kernel_memory.h"
#include "objects/vm_area.h"
#include "vm_space.h"

using memory::frame_size;

vm_area::vm_area(size_t size, vm_flags flags) :
	m_size {size},
	m_flags {flags},
	kobject {kobject::type::vma}
{
}

vm_area::~vm_area() {}

size_t
vm_area::resize(size_t size)
{
	if (mapper().can_resize(size))
		m_size = size;
	return m_size;
}

void
vm_area::commit(uintptr_t phys, uintptr_t offset, size_t count)
{
	mapper().map(offset, count, phys);
}

void
vm_area::uncommit(uintptr_t offset, size_t count)
{
	mapper().unmap(offset, count);
}

void
vm_area::on_no_handles()
{
	kobject::on_no_handles();
	delete this;
}



vm_area_shared::vm_area_shared(size_t size, vm_flags flags) :
	m_mapper {*this},
	vm_area {size, flags}
{
}

vm_area_shared::~vm_area_shared()
{
}

size_t
vm_area_shared::overlaps(uintptr_t offset, size_t pages, size_t *count)
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

void
vm_area_shared::commit(uintptr_t phys, uintptr_t offset, size_t count)
{
	size_t n = 0;
	size_t o = overlaps(offset, count, &n);

	// Mapping overlaps not allowed
	if (n) return;

	m_mapper.map(offset, count, phys);

	o = m_mappings.sorted_insert({
		.offset = offset,
		.count = count,
		.phys = phys});
	n = 1;


	// Try to expand to abutting similar areas
	if (o > 0 &&
		m_mappings[o-1].end() == offset &&
		m_mappings[o-1].phys_end() == phys) {
		--o;
		++n;
	}

	uintptr_t end = offset + count * frame_size;
	uintptr_t pend = phys + count * frame_size;

	if (o + n < m_mappings.count() &&
		end == m_mappings[o+n].offset &&
		m_mappings[o-1].phys == pend) {
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

	first.count = memory::page_count(diff);

	if (n > 1)
		m_mappings.remove_at(o+1, n-1);

	return;
}

void
vm_area_shared::uncommit(uintptr_t offset, size_t count)
{
	size_t n = 0;
	size_t o = overlaps(offset, count, &n);
	if (!n) return;

	m_mapper.unmap(offset, count);

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
			});
		last = &m_mappings[i];
		trailing = 0;

		first->count -= last->count;
		last->phys = first->phys + first->count * frame_size;
	}

	if (leading) {
		size_t remove_pages = first->count; 
		first->count = leading / frame_size;
		remove_pages -= first->count;
		m_mapper.unmap(first->end(), remove_pages);
	}

	if (trailing) {
		uintptr_t remove_off = last->offset;
		size_t remove_pages = last->count; 
		last->offset = end;
		last->count = trailing / frame_size;
		remove_pages -= last->count;
		m_mapper.unmap(remove_off, remove_pages);
		last->phys += remove_pages * frame_size;
	}

	size_t delete_start = 0;
	size_t delete_count = 0;
	for (size_t i = o; i < o+n; ++i) {
		mapping &m = m_mappings[i];
		if (offset <= m.offset && end >= m.end()) {
			if (!delete_count) delete_start = i;
			++delete_count;
			m_mapper.unmap(m.offset, m.count);
		}
	}

	if (delete_count)
		m_mappings.remove_at(delete_start, delete_count);

	return;
}


vm_area_open::vm_area_open(size_t size, vm_space &space, vm_flags flags) :
	m_mapper(*this, space),
	vm_area(size, flags)
{
}

void
vm_area_open::commit(uintptr_t phys, uintptr_t offset, size_t count)
{
	m_mapper.map(offset, count, phys);
}

void
vm_area_open::uncommit(uintptr_t offset, size_t count)
{
	m_mapper.unmap(offset, count);
}

