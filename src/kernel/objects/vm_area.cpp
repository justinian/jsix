
#include "assert.h"
#include "frame_allocator.h"
#include "memory.h"
#include "objects/vm_area.h"
#include "page_tree.h"
#include "vm_space.h"

namespace obj {

using mem::frame_size;

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

void
vm_area::remove_from(vm_space *space)
{
    m_spaces.remove_swap(space);
    if (!m_spaces.count() && !handle_count())
        delete this;
}

void
vm_area::on_no_handles()
{
    if (!m_spaces.count())
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

vm_area_fixed::~vm_area_fixed()
{
    if (m_flags && vm_flags::mmio)
        return;

    size_t pages = mem::page_count(m_size);
    frame_allocator::get().free(m_start, pages);
}

size_t
vm_area_fixed::resize(size_t size)
{
    // Not resizable
    return m_size;
}

bool
vm_area_fixed::get_page(uintptr_t offset, uintptr_t &phys)
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

vm_area_untracked::~vm_area_untracked()
{
    kassert(false, "An untracked VMA's pages cannot be reclaimed, leaking memory");
}

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

vm_area_open::~vm_area_open()
{
    // the page_tree will free its pages when deleted
    delete m_mapped;
}

bool
vm_area_open::get_page(uintptr_t offset, uintptr_t &phys)
{
    return page_tree::find_or_add(m_mapped, offset, phys);
}


vm_area_guarded::vm_area_guarded(uintptr_t start, size_t buf_pages, size_t size, vm_flags flags) :
    m_pages {buf_pages + 1}, // Sections are N+1 pages for the leading guard page
    m_stacks {start, m_pages*mem::frame_size},
    vm_area_open {size, flags}
{
}

vm_area_guarded::~vm_area_guarded() {}

uintptr_t
vm_area_guarded::get_section()
{
    // Account for the leading guard page
    return m_stacks.allocate() + mem::frame_size;
}

void
vm_area_guarded::return_section(uintptr_t addr)
{
    // Account for the leading guard page
    return m_stacks.free(addr - mem::frame_size);
}

bool
vm_area_guarded::get_page(uintptr_t offset, uintptr_t &phys)
{
    if (offset >= m_stacks.end())
        return false;

    // make sure this isn't in a guard page. (sections have 1 leading
    // guard page, so page 0 is invalid)
    if ((offset >> 12) % m_pages == 0)
        return false;

    return vm_area_open::get_page(offset, phys);
}

} // namespace obj
