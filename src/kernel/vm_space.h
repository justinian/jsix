#pragma once
/// \file vm_space.h
/// Structure for tracking a range of virtual memory addresses

#include <stdint.h>

#include <j6/flags.h>
#include <util/bitset.h>
#include <util/spinlock.h>
#include <util/vector.h>

#include "objects/vm_area.h"
#include "page_table.h"

struct TCB;

namespace obj {
    class process;
}

/// Tracks a region of virtual memory address space
class vm_space
{
public:
    /// Never map below this address unless explicitly asked.
    static constexpr uintptr_t min_auto_address = 16 * 1024 * 1024; // 16 MiB

    /// Constructor for the kernel address space
    /// \arg pml4   The existing kernel PML4
    vm_space(page_table *pml4);

    /// Constructor. Creates a new address space.
    vm_space();

    ~vm_space();

    /// Add a virtual memorty area to this address space
    /// \arg base  The starting address of the area
    /// \arg area  The area to add
    /// \arg flags Flags for the operation (exact, clobber, etc)
    /// \returns   The base address the area was added at
    uintptr_t add(uintptr_t base, obj::vm_area *area, util::bitset32 flags);

    /// Remove a virtual memory area from this address space
    /// \arg area  The area to remove
    /// \returns   True if the area was removed
    bool remove(obj::vm_area *area);

    /// Get the virtual memory area corresponding to an address
    /// \arg addr  The address to check
    /// \arg base  [out] if not null, receives the base address of the area
    /// \returns   The vm_area, or nullptr if not found
    obj::vm_area * get(uintptr_t addr, uintptr_t *base = nullptr);

    /// Check if this is the kernel space
    inline bool is_kernel() const { return m_kernel; }

    /// Get the kernel virtual memory space
    static vm_space & kernel_space();

    /// Map virtual addressses to the given physical pages
    /// \arg area   The VMA this mapping applies to
    /// \arg offset Offset of the starting virutal address from the VMA base
    /// \arg phys   The starting physical address
    /// \arg count  The number of contiugous physical pages to map
    void page_in(const obj::vm_area &area, uintptr_t offset, uintptr_t phys, size_t count);

    /// Clear mappings from the given region
    /// \arg area   The VMA these mappings applies to
    /// \arg offset Offset of the starting virutal address from the VMA base
    /// \arg count  The number of pages worth of mappings to clear
    /// \arg free   If true, free the pages back to the system
    void clear(const obj::vm_area &vma, uintptr_t offset, size_t count, bool free = false);

    /// Clear mappings from the given region, and mark it as locked. Used for
    /// debugging heap allocation reuse.
    /// \arg area   The VMA these mappings applies to
    /// \arg offset Offset of the starting virutal address from the VMA base
    /// \arg count  The number of pages worth of mappings to clear
    void lock(const obj::vm_area &vma, uintptr_t offset, size_t count);

    /// Look up the address of a given VMA's offset
    uintptr_t lookup(const obj::vm_area &vma, uintptr_t offset);

    /// Check if this space is the current active space
    bool active() const;

    /// Set this space as the current active space
    void activate() const;

    /// Allocate pages into virtual memory. May allocate less than requested.
    /// \arg virt  The virtual address at which to allocate
    /// \arg count The number of pages to allocate
    /// \arg phys  [out] The physical address of the pages allocated
    /// \returns   The number of pages actually allocated
    size_t allocate(uintptr_t virt, size_t count, uintptr_t *phys);

    enum class fault_type { present, write, user, reserved, fetch };

    /// Handle a page fault.
    /// \arg addr  Address which caused the fault
    /// \arg ft    Flags from the interrupt about the kind of fault
    /// \returns   True if the fault was successfully handled
    bool handle_fault(uintptr_t addr, util::bitset8 fault);

    /// Set up a TCB to operate in this address space.
    void initialize_tcb(TCB &tcb);

    /// Copy data from one address space to another
    /// \arg source The address space data is being copied from
    /// \arg dest   The address space data is being copied to
    /// \arg from   Pointer to the data in the source address space
    /// \arg to     Pointer to the destination in the dest address space
    /// \arg length Amount of data to copy, in bytes
    /// \returnd    The number of bytes copied
    static size_t copy(vm_space &source, vm_space &dest, const void *from, void *to, size_t length);

    /// Get the physical address of a virtual address from this space
    /// \arg vrit  The virtual address
    /// \returns   The physical address mapped to that virtual address,
    ///            or 0 for none.
    uintptr_t find_physical(uintptr_t virt);

private:
    friend class obj::vm_area;

    /// Find a given VMA in this address space
    bool find_vma(const obj::vm_area &vma, uintptr_t &base) const;

    /// Check if a VMA can be resized
    bool can_resize(const obj::vm_area &vma, size_t size) const;

    /// Copy a range of mappings from the given address space 
    void copy_from(const vm_space &source, const obj::vm_area &vma);

    /// Remove an area's mappings from this space
    void remove_area(obj::vm_area *area);

    bool m_kernel;
    page_table *m_pml4;

    struct area {
        uintptr_t base;
        obj::vm_area *area;
        int compare(const struct area &o) const;
        bool operator==(const struct area &o) const;
    };
    util::vector<area> m_areas;

    util::spinlock m_lock;
};