#include "frame_allocator.h"
#include "kernel_memory.h"
#include "objects/vm_area.h"
#include "page_tree.h"
#include "vm_space.h"

using memory::frame_size;

vm_area::vm_area(size_t size, vm_flags flags) :
	m_size {size},
	m_flags {flags},
	m_spaces {m_vector_static, 0, static_size},
	kobject {kobject::type::vma}
{
}

vm_area::~vm_area() {}

bool
vm_area::add_to(vm_space *space)
{
	for (auto *s : m_spaces) {
		if (s == space)
			return true;
	}
	m_spaces.append(space);
	return true;
}

bool
vm_area::remove_from(vm_space *space)
{
	m_spaces.remove_swap(space);
	return
		!m_spaces.count() &&
		!(m_flags && vm_flags::mmio);
}

void
vm_area::on_no_handles()
{
	kobject::on_no_handles();
	delete this;
}

size_t
vm_area::resize(size_t size)
{
	if (can_resize(size))
		m_size = size;
	return m_size;
}

bool
vm_area::can_resize(size_t size)
{
	for (auto *space : m_spaces)
		if (!space->can_resize(*this, size))
			return false;
	return true;
}

vm_area_fixed::vm_area_fixed(uintptr_t start, size_t size, vm_flags flags) :
	m_start {start},
	vm_area {size, flags}
{
}

vm_area_fixed::~vm_area_fixed() {}

size_t vm_area_fixed::resize(size_t size)
{
	// Not resizable
	return m_size;
}

bool vm_area_fixed::get_page(uintptr_t offset, uintptr_t &phys)
{
	if (offset > m_size)
		return false;

	phys = m_start + offset;
	return true;
}


vm_area_untracked::vm_area_untracked(size_t size, vm_flags flags) :
	vm_area {size, flags}
{
}

vm_area_untracked::~vm_area_untracked() {}

bool
vm_area_untracked::get_page(uintptr_t offset, uintptr_t &phys)
{
	if (offset > m_size)
		return false;

	return frame_allocator::get().allocate(1, &phys);
}

bool
vm_area_untracked::add_to(vm_space *space)
{
	if (!m_spaces.count())
		return vm_area::add_to(space);
	return m_spaces[0] == space;
}


vm_area_open::vm_area_open(size_t size, vm_flags flags) :
	m_mapped {nullptr},
	vm_area {size, flags}
{
}

vm_area_open::~vm_area_open() {}

bool
vm_area_open::get_page(uintptr_t offset, uintptr_t &phys)
{
	return page_tree::find_or_add(m_mapped, offset, phys);
}


vm_area_guarded::vm_area_guarded(uintptr_t start, size_t buf_pages, size_t size, vm_flags flags) :
	m_start {start},
	m_pages {buf_pages},
	m_next {memory::frame_size},
	vm_area_untracked {size, flags}
{
}

vm_area_guarded::~vm_area_guarded() {}

uintptr_t
vm_area_guarded::get_section()
{
	if (m_cache.count() > 0) {
		return m_cache.pop();
	}

	uintptr_t addr = m_next;
	m_next += (m_pages + 1) * memory::frame_size;
	return m_start + addr;
}

void
vm_area_guarded::return_section(uintptr_t addr)
{
	m_cache.append(addr);
}

bool
vm_area_guarded::get_page(uintptr_t offset, uintptr_t &phys)
{
	if (offset > m_next)
		return false;

	// make sure this isn't in a guard page. (sections are
	// m_pages big plus 1 leading guard page, so page 0 is
	// invalid)
	if ((offset >> 12) % (m_pages+1) == 0)
		return false;

	return vm_area_untracked::get_page(offset, phys);
}

