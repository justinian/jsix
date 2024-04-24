#include <j6/memutils.h>
#include <arch/memory.h>

#include "kassert.h"
#include "frame_allocator.h"
#include "logger.h"
#include "memory.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "objects/vm_area.h"
#include "sysconf.h"
#include "vm_space.h"

using obj::vm_flags;

// The initial memory for the array of areas for the kernel space
static constexpr size_t num_kernel_areas = 8;
static uint64_t kernel_areas[num_kernel_areas * 2];

static constexpr uint64_t locked_page_tag = 0xbadfe11a;

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

    memset(m_pml4, 0, mem::frame_size/2);
    for (unsigned i = arch::kernel_root_index; i < arch::table_entries; ++i)
        m_pml4->entries[i] = kpml4->entries[i];

    // Every vm space has sysconf in it
    obj::vm_area *sysc = new obj::vm_area_fixed(
            g_sysconf_phys,
            sizeof(system_config),
            0);

    add(sysconf_user_address, sysc, util::bitset32::of(vm_flags::exact));
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
    return obj::process::kernel_process().space();
}

uintptr_t
vm_space::add(uintptr_t base, obj::vm_area *area, util::bitset32 flags)
{
    if (!base)
        base = min_auto_address;

    //TODO: optimize find/insert
    bool exact = flags.get(vm_flags::exact);
    for (size_t i = 0; i < m_areas.count(); ++i) {
        const vm_space::area &a = m_areas[i];
        uintptr_t aend = a.base + a.area->size();
        if (base >= aend)
            continue;

        uintptr_t end = base + area->size();
        if (end <= a.base)
            break;
        else if (exact)
            return 0;
        else
            base = aend;
    }

    m_areas.sorted_insert({base, area});
    area->add_to(this);
    area->handle_retain();
    return base;
}

void
vm_space::remove_area(obj::vm_area *area)
{
    area->remove_from(this);
    clear(*area, 0, mem::page_count(area->size()));
    area->handle_release();
}

bool
vm_space::remove(obj::vm_area *area)
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
vm_space::can_resize(const obj::vm_area &vma, size_t size) const
{
    uintptr_t base = 0;
    unsigned n = m_areas.count();
    for (unsigned i = 1; i < n - 1; ++i) {
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

obj::vm_area *
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
vm_space::find_vma(const obj::vm_area &vma, uintptr_t &base) const
{
    for (auto &a : m_areas) {
        if (a.area != &vma) continue;
        base = a.base;
        return true;
    }
    return false;
}

void
vm_space::copy_from(const vm_space &source, const obj::vm_area &vma)
{
    uintptr_t to = 0;
    uintptr_t from = 0;
    if (!find_vma(vma, from) || !source.find_vma(vma, from))
        return;

    size_t count = mem::page_count(vma.size());
    page_table::iterator dit {to, m_pml4};
    page_table::iterator sit {from, source.m_pml4};

    while (count--) {
        uint64_t &e = dit.entry(page_table::level::pt);
        if (util::bitset64::from(e) & page_flags::present) {
            // TODO: handle clobbering mapping
        }
        e = sit.entry(page_table::level::pt);
    }
}

void
vm_space::page_in(const obj::vm_area &vma, uintptr_t offset, uintptr_t phys, size_t count)
{
    using mem::frame_size;
    util::scoped_lock lock {m_lock};

    uintptr_t base = 0;
    if (!find_vma(vma, base))
        return;

    uintptr_t virt = base + offset;
    util::bitset64 flags =
        page_flags::present |
        (m_kernel ? page_flags::none : page_flags::user) |
        (vma.flags().get(vm_flags::write) ? page_flags::write : page_flags::none) |
        (vma.flags().get(vm_flags::write_combine) ? page_flags::wc : page_flags::none);

    page_table::iterator it {virt, m_pml4};

    for (size_t i = 0; i < count; ++i) {
        uint64_t &entry = it.entry(page_table::level::pt);
        entry = (phys + i * frame_size) | flags;
        log::spam(logs::paging, "Setting entry for %016llx: %016llx [%04llx]",
                it.vaddress(), (phys + i * frame_size), flags.value());
        ++it;
    }
}

void
vm_space::clear(const obj::vm_area &vma, uintptr_t offset, size_t count, bool free)
{
    using mem::frame_size;
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
        util::bitset64 flags = e;

        if (flags & page_flags::present) {
            e = 0;
            if (flags & page_flags::accessed) {
                auto *addr = reinterpret_cast<const uint8_t *>(it.vaddress());
                asm ( "invlpg %0" :: "m"(*addr) : "memory" );
            }
            if (free_count && phys == free_start + (free_count * frame_size)) {
                ++free_count;
            } else {
                if (free && free_count)
                    fa.free(free_start, free_count);
                free_start = phys;
                free_count = 1;
            }
        }

        ++it;
    }

    if (free && free_count)
        fa.free(free_start, free_count);
}

void
vm_space::lock(const obj::vm_area &vma, uintptr_t offset, size_t count)
{
    using mem::frame_size;
    util::scoped_lock lock {m_lock};

    uintptr_t base = 0;
    if (!find_vma(vma, base))
        return;

    uintptr_t addr = base + offset;

    frame_allocator &fa = frame_allocator::get();
    page_table::iterator it {addr, m_pml4};

    while (count--) {
        uint64_t &e = it.entry(page_table::level::pt);
        uintptr_t phys = e & ~0xfffull;
        util::bitset64 flags = e;

        if (flags & page_flags::present) {
            e = locked_page_tag;
            if (flags & page_flags::accessed) {
                auto *addr = reinterpret_cast<const uint8_t *>(it.vaddress());
                asm ( "invlpg %0" :: "m"(*addr) : "memory" );
            }
            fa.free(phys, 1);
        }
        ++it;
    }
}

uintptr_t
vm_space::lookup(const obj::vm_area &vma, uintptr_t offset)
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
    return mem::to_virtual<page_table>(pml4 & ~0xfffull) == m_pml4;
}

void
vm_space::activate() const
{
    constexpr uint64_t phys_mask = ~mem::linear_offset & ~0xfffull;
    uintptr_t p = reinterpret_cast<uintptr_t>(m_pml4) & phys_mask;
    __asm__ __volatile__ ( "mov %0, %%cr3" :: "r" (p) );
}

void
vm_space::initialize_tcb(TCB &tcb)
{
    tcb.pml4 =
        reinterpret_cast<uintptr_t>(m_pml4) &
        ~mem::linear_offset;
}

bool
vm_space::handle_fault(uintptr_t addr, util::bitset8 fault)
{
    // TODO: Handle more fult types
    if (fault.get(fault_type::present))
        return false;

    uintptr_t page = (addr & ~0xfffull);

    uintptr_t base = 0;
    obj::vm_area *area = get(addr, &base);
    if (!area)
        return false;

    if constexpr (__debug_heap_allocation) {
        page_table::iterator it {addr, m_pml4};
        uint64_t &e = it.entry(page_table::level::pt);
        kassert(e != locked_page_tag, "Use-after-free");
        if (e == locked_page_tag)
            return false;
    }

    uintptr_t offset = page - base;
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
        mem::to_virtual<void>((*dit & ~0xfffull) | (ito & 0xffful)),
        mem::to_virtual<void>((*sit & ~0xfffull) | (ifrom & 0xffful)),
        length);

    return length;
}

uintptr_t
vm_space::find_physical(uintptr_t virt)
{
    uintptr_t base = 0;
    obj::vm_area *area = get(virt, &base);
    if (!area)
        return 0;

    uintptr_t phys = 0;
    area->get_page(virt - base, phys, false);
    return phys;
}

