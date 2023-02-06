#include <arch/memory.h>
#include <bootproto/memory.h>
#include <util/counted.h>
#include <util/pointers.h>

#include "allocator.h"
#include "error.h"
#include "loader.h"
#include "memory.h"
#include "paging.h"
#include "status.h"

namespace boot {
namespace paging {

using memory::alloc_type;
using memory::page_size;

// Flags: 0 0 0 1  0 0 0 0  0 0 0 1 = 0x0101
//         IGN  |  | | | |  | | | +- Present
//              |  | | | |  | | +--- Writeable
//              |  | | | |  | +----- Usermode access (Supervisor only)
//              |  | | | |  +------- PWT (determining memory type for page)
//              |  | | | +---------- PCD (determining memory type for page)
//              |  | | +------------ Accessed flag (not accessed yet)
//              |  | +-------------- Dirty (not dirtied yet)
//              |  +---------------- PAT (determining memory type for page)
//              +------------------- Global
/// Page table entry flags for entries pointing at a page
constexpr uint64_t page_flags = 0x101;

// Flags: 0  0 0 0 1  1 0 0 0  1 0 1 1 = 0x018b
//        |   IGN  |  | | | |  | | | +- Present
//        |        |  | | | |  | | +--- Writeable
//        |        |  | | | |  | +----- Usermode access (Supervisor only)
//        |        |  | | | |  +------- PWT (determining memory type for page)
//        |        |  | | | +---------- PCD (determining memory type for page)
//        |        |  | | +------------ Accessed flag (not accessed yet)
//        |        |  | +-------------- Dirty (not dirtied yet)
//        |        |  +---------------- Page size (1GiB page)
//        |        +------------------- Global
//        +---------------------------- PAT (determining memory type for page)
/// Page table entry flags for entries pointing at a huge page
constexpr uint64_t huge_page_flags = 0x18b;

// Flags: 0 0 0 0  0 0 0 0  0 0 1 1 = 0x0003
//        IGNORED  | | | |  | | | +- Present
//                 | | | |  | | +--- Writeable
//                 | | | |  | +----- Usermode access (Supervisor only)
//                 | | | |  +------- PWT (determining memory type for pdpt)
//                 | | | +---------- PCD (determining memory type for pdpt)
//                 | | +------------ Accessed flag (not accessed yet)
//                 | +-------------- Ignored
//                 +---------------- Reserved 0 (Table pointer, not page)
/// Page table entry flags for entries pointing at another table
constexpr uint64_t table_flags = 0x003;


/// Struct to allow easy accessing of a memory page being used as a page table.
struct page_table
{
    uint64_t entries[512];

    inline page_table * get(int i, uint16_t *flags = nullptr) const {
        uint64_t entry = entries[i];
        if ((entry & 1) == 0) return nullptr;
        if (flags) *flags = entry & 0xfff;
        return reinterpret_cast<page_table *>(entry & ~0xfffull);
    }

    inline void set(int i, void *p, uint16_t flags) {
        entries[i] = reinterpret_cast<uint64_t>(p) | (flags & 0xfff);
    }
};

/// Iterator over page table entries.
template <unsigned D = 4>
class page_entry_iterator
{
public:
    /// Constructor.
    /// \arg virt      Virtual address this iterator is starting at
    /// \arg pgr       The pager, used for its page table cache
    page_entry_iterator(
            uintptr_t virt,
            pager &pgr) :
        m_pager(pgr)
    {
        m_table[0] = pgr.m_pml4;
        for (unsigned i = 0; i < D; ++i) {
            m_index[i] = static_cast<uint16_t>((virt >> (12 + 9*(3-i))) & 0x1ff);
            ensure_table(i);
        }
    }

    uintptr_t vaddress(unsigned level = D) const {
        uintptr_t address = 0;
        for (unsigned i = 0; i < level; ++i)
            address |= static_cast<uintptr_t>(m_index[i]) << (12 + 9*(3-i));
        if (address & (1ull<<47)) // canonicalize the address
            address |= (0xffffull<<48);
        return address;
    }

    void increment()
    {
        for (unsigned i = D - 1; i >= 0; --i) {
            if (++m_index[i] <= 511) {
                for (unsigned j = i + 1; j < D; ++j)
                    ensure_table(j);
                return;
            }

            m_index[i] = 0;
        }
    }

    uint64_t & operator*() { return entry(D-1); }
    uint64_t operator[](int i) { return i < D ? m_index[i] : 0; }

private:
    inline uint64_t & entry(unsigned level) { return m_table[level]->entries[m_index[level]]; }

    void ensure_table(unsigned level)
    {
        // We're only dealing with D levels of paging, and
        // there must always be a PML4.
        if (level < 1 || level >= D)
            return;

        // Entry in the parent that points to the table we want
        uint64_t & parent_ent = entry(level - 1);

        if (!(parent_ent & 1)) {
            page_table *table = reinterpret_cast<page_table*>(m_pager.pop_pages(1));
            parent_ent = (reinterpret_cast<uintptr_t>(table) & ~0xfffull) | table_flags;
            m_table[level] = table;
        } else {
            m_table[level] = reinterpret_cast<page_table*>(parent_ent & ~0xfffull);
        }
    }

    pager &m_pager;
    page_table *m_table[D];
    uint16_t m_index[D];
};

pager::pager(uefi::boot_services *bs) :
    m_bs {bs}
{
    status_line status(L"Allocating initial page tables");

    // include 1 extra for kernel PML4
    static constexpr size_t tables_needed = pd_tables + extra_tables + 1;

    void *addr = g_alloc.allocate_pages(tables_needed, alloc_type::page_table, true);
    m_bs->set_mem(addr, tables_needed * page_size, 0);
    m_table_pages = { .pointer = addr, .count = tables_needed };

    create_kernel_tables();
}

void
pager::map_pages(
    uintptr_t phys, uintptr_t virt, size_t count,
    bool write_flag, bool exe_flag)
{
    if (!count)
        return;

    page_entry_iterator<4> iterator {virt, *this};

    uint64_t flags = page_flags;
    if (!exe_flag)  flags |= (1ull << 63); // set NX bit
    if (write_flag) flags |= 2;

    while (true) {
        uint64_t entry = phys | flags;
        *iterator = entry;

        if (--count == 0)
            break;

        iterator.increment();
        phys += page_size;
    }
}

void
pager::update_kernel_args(bootproto::args *args)
{
    status_line status {L"Updating kernel args"};
    args->pml4 = reinterpret_cast<void*>(m_pml4);
    args->page_tables = m_table_pages;
}

void
pager::add_current_mappings()
{
    // Get the pointer to the current PML4
    page_table *old_pml4 = 0;
    asm volatile ( "mov %%cr3, %0" : "=r" (old_pml4) );

    // Only copy mappings in the lower half
    constexpr unsigned halfway = arch::kernel_root_index; 
    for (int i = 0; i < halfway; ++i) {
        uint64_t entry = old_pml4->entries[i];
        if (entry & 1)
            m_pml4->entries[i] = entry;
    }
}

page_table *
pager::pop_pages(size_t count)
{
    if (count > m_table_pages.count)
        error::raise(uefi::status::out_of_resources, L"Page table cache empty", 0x7ab1e5);

    page_table *next = reinterpret_cast<page_table*>(m_table_pages.pointer);
    m_table_pages.pointer = util::offset_pointer(m_table_pages.pointer, count*page_size);
    m_table_pages.count -= count;
    return next;
}

void
pager::create_kernel_tables()
{
    m_pml4 = pop_pages(1);
    page_table *pds = pop_pages(pd_tables);

    // Add PDs for all of high memory into the PML4
    constexpr unsigned start = arch::kernel_root_index; 
    constexpr unsigned end = arch::table_entries; 
    for (unsigned i = start; i < end; ++i)
        m_pml4->set(i, &pds[i - start], table_flags);

    // Add the linear offset-mapped area
    uintptr_t phys = 0;
    uintptr_t virt_base = bootproto::mem::linear_offset;
    size_t page_count = 64 * 1024; // 64 TiB of 1 GiB pages
    constexpr size_t GiB = 0x40000000ull;

    page_entry_iterator<2> iterator {virt_base, *this};
    while (true) {
        *iterator = phys | huge_page_flags;
        if (--page_count == 0)
            break;
        iterator.increment();
        phys += GiB;
    }
}

} // namespace paging
} // namespace boot
