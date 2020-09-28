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


vm_area_buffers::vm_area_buffers(size_t size, vm_space &space, vm_flags flags, size_t buf_pages) :
	m_mapper {*this, space},
	m_pages {buf_pages},
	m_next {memory::frame_size},
	vm_area {size, flags}
{
}

uintptr_t
vm_area_buffers::get_buffer()
{
	if (m_cache.count() > 0) {
		return m_cache.pop();
	}

	uintptr_t addr = m_next;
	m_next += (m_pages + 1) * memory::frame_size;
	return m_mapper.space().lookup(*this, addr);
}

void
vm_area_buffers::return_buffer(uintptr_t addr)
{
	m_cache.append(addr);
}

bool
vm_area_buffers::allowed(uintptr_t offset) const
{
	if (offset >= m_next) return false;

	// Buffers are m_pages big plus 1 leading guard page
	return memory::page_align_down(offset) % (m_pages+1);
}

void
vm_area_buffers::commit(uintptr_t phys, uintptr_t offset, size_t count)
{
	m_mapper.map(offset, count, phys);
}

void
vm_area_buffers::uncommit(uintptr_t offset, size_t count)
{
	m_mapper.unmap(offset, count);
}

