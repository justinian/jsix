#include <string.h>

#include "kernel_memory.h"

#include "assert.h"
#include "frame_allocator.h"
#include "log.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "objects/vm_area.h"
#include "vm_space.h"

// The initial memory for the array of areas for the kernel space
constexpr size_t num_kernel_areas = 8;
static uint64_t kernel_areas[num_kernel_areas * 2];

int
vm_space::area::compare(const vm_space::area &o) const
{
    if (base > o.base) return 1;
    else if (base < o.base) return -1;
    else return 0;
}

bool
vm_space::area::operator==(const vm_space::area &o) const
{
    return o.base == base && o.area == area;
}


// Kernel address space contsructor
vm_space::vm_space(page_table *p) :
    m_kernel {true},
    m_pml4 {p},
    m_areas {reinterpret_cast<vm_space::area*>(kernel_areas), 0, num_kernel_areas}
{}

vm_space::vm_space() :
    m_kernel {false}
{
    m_pml4 = page_table::get_table_page();
    page_table *kpml4 = kernel_space().m_pml4;

    memset(m_pml4, 0, memory::frame_size/2);
    for (unsigned i = memory::pml4e_kernel; i < memory::table_entries; ++i)
        m_pml4->entries[i] = kpml4->entries[i];
}

vm_space::~vm_space()
{
    for (auto &a : m_areas)
        remove_area(a.area);

    kassert(!is_kernel(), "Kernel vm_space destructor!");
    if (active())
        kernel_space().activate();

    // All VMAs have been removed by now, so just
    // free all remaining pages and tables
    m_pml4->free(page_table::level::pml4);
}

vm_space &
vm_space::kernel_space()
{
    return process::kernel_process().space();
}

bool
vm_space::add(uintptr_t base, vm_area *area)
{
    //TODO: check for collisions
    m_areas.sorted_insert({base, area});
    area->add_to(this);
    area->handle_retain();
    return true;
}

void
vm_space::remove_area(vm_area *area)
{
    area->remove_from(this);
    clear(*area, 0, memory::page_count(area->size()));
    area->handle_release();
}

bool
vm_space::remove(vm_area *area)
{
    for (auto &a : m_areas) {
        if (a.area == area) {
            remove_area(area);
            m_areas.remove(a);
            return true;
        }
    }
    return false;
}

bool
vm_space::can_resize(const vm_area &vma, size_t size) const
{
    uintptr_t base = 0;
    unsigned n = m_areas.count();
    for (unsigned i = 0; i < n - 1; ++i) {
        const area &prev = m_areas[i - 1];
        if (prev.area != &vma)
            continue;

        base = prev.base;
        const area &next = m_areas[i];
        if (prev.base + size > next.base)
            return false;
    }

    uintptr_t end = base + size;
    uintptr_t space_end = is_kernel() ?
        uint64_t(-1) : 0x7fffffffffff;

    return end <= space_end;
}

vm_area *
vm_space::get(uintptr_t addr, uintptr_t *base)
{
    for (auto &a : m_areas) {
        uintptr_t end = a.base + a.area->size();
        if (addr >= a.base && addr < end) {
            if (base) *base = a.base;
            return a.area;
        }
    }
    return nullptr;
}

bool
vm_space::find_vma(const vm_area &vma, uintptr_t &base) const
{
    for (auto &a : m_areas) {
        if (a.area != &vma) continue;
        base = a.base;
        return true;
    }
    return false;
}

void
vm_space::copy_from(const vm_space &source, const vm_area &vma)
{
    uintptr_t to = 0;
    uintptr_t from = 0;
    if (!find_vma(vma, from) || !source.find_vma(vma, from))
        return;

    size_t count = memory::page_count(vma.size());
    page_table::iterator dit {to, m_pml4};
    page_table::iterator sit {from, source.m_pml4};

    while (count--) {
        uint64_t &e = dit.entry(page_table::level::pt);
        if (e & page_table::flag::present) {
            // TODO: handle clobbering mapping
        }
        e = sit.entry(page_table::level::pt);
    }
}

void
vm_space::page_in(const vm_area &vma, uintptr_t offset, uintptr_t phys, size_t count)
{
    using memory::frame_size;
    util::scoped_lock lock {m_lock};

    uintptr_t base = 0;
    if (!find_vma(vma, base))
        return;

    uintptr_t virt = base + offset;
    page_table::flag flags =
        page_table::flag::present |
        (m_kernel ? page_table::flag::none : page_table::flag::user) |
        ((vma.flags() && vm_flags::write) ? page_table::flag::write : page_table::flag::none) |
        ((vma.flags() && vm_flags::write_combine) ? page_table::flag::wc : page_table::flag::none);

    page_table::iterator it {virt, m_pml4};

    for (size_t i = 0; i < count; ++i) {
        uint64_t &entry = it.entry(page_table::level::pt);
        entry = (phys + i * frame_size) | flags;
        log::debug(logs::paging, "Setting entry for %016llx: %016llx [%04llx]",
                it.vaddress(), (phys + i * frame_size), flags);
        ++it;
    }
}

void
vm_space::clear(const vm_area &vma, uintptr_t offset, size_t count, bool free)
{
    using memory::frame_size;
    util::scoped_lock lock {m_lock};

    uintptr_t base = 0;
    if (!find_vma(vma, base))
        return;

    uintptr_t addr = base + offset;
    uintptr_t free_start = 0;
    size_t free_count = 0;

    frame_allocator &fa = frame_allocator::get();
    page_table::iterator it {addr, m_pml4};

    while (count--) {
        uint64_t &e = it.entry(page_table::level::pt);
        uintptr_t phys = e & ~0xfffull;

        if (e & page_table::flag::present) {
            if (free_count && phys == free_start + (free_count * frame_size)) {
                ++free_count;
            } else {
                if (free && free_count)
                    fa.free(free_start, free_count);
                free_start = phys;
                free_count = 1;
            }
        }

        e = 0;
        ++it;
    }

    if (free && free_count)
        fa.free(free_start, free_count);
}

uintptr_t
vm_space::lookup(const vm_area &vma, uintptr_t offset)
{
    uintptr_t base = 0;
    if (!find_vma(vma, base))
        return 0;
    return base + offset;
}

bool
vm_space::active() const
{
    uintptr_t pml4 = 0;
    __asm__ __volatile__ ( "mov %%cr3, %0" : "=r" (pml4) );
    return memory::to_virtual<page_table>(pml4 & ~0xfffull) == m_pml4;
}

void
vm_space::activate() const
{
    constexpr uint64_t phys_mask = ~memory::page_offset & ~0xfffull;
    uintptr_t p = reinterpret_cast<uintptr_t>(m_pml4) & phys_mask;
    __asm__ __volatile__ ( "mov %0, %%cr3" :: "r" (p) );
}

void
vm_space::initialize_tcb(TCB &tcb)
{
    tcb.pml4 =
        reinterpret_cast<uintptr_t>(m_pml4) &
        ~memory::page_offset;
}

bool
vm_space::handle_fault(uintptr_t addr, fault_type fault)
{
    // TODO: Handle more fult types
    if (fault && fault_type::present)
        return false;

    uintptr_t base = 0;
    vm_area *area = get(addr, &base);
    if (!area)
        return false;

    uintptr_t offset = (addr & ~0xfffull) - base;
    uintptr_t phys_page = 0;
    if (!area->get_page(offset, phys_page))
        return false;

    page_in(*area, offset, phys_page, 1);
    return true;
}

size_t
vm_space::copy(vm_space &source, vm_space &dest, const void *from, void *to, size_t length)
{
    uintptr_t ifrom = reinterpret_cast<uintptr_t>(from);
    uintptr_t ito = reinterpret_cast<uintptr_t>(to);

    page_table::iterator sit {ifrom, source.m_pml4};
    page_table::iterator dit {ito, dest.m_pml4};

    // TODO: iterate page mappings and continue copying. For now i'm blindly
    // assuming both buffers are fully contained within single pages
    memcpy(
        memory::to_virtual<void>((*dit & ~0xfffull) | (ito & 0xffful)),
        memory::to_virtual<void>((*sit & ~0xfffull) | (ifrom & 0xffful)),
        length);

    return length;
}
