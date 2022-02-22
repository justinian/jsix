#pragma once
/// \file vm_area.h
/// Definition of VMA objects and related functions

#include <stddef.h>
#include <stdint.h>

#include <j6/caps.h>
#include <util/vector.h>
#include <util/enum_bitfields.h>

#include "objects/kobject.h"

class page_tree;
class vm_space;


namespace obj {

enum class vm_flags : uint32_t
{
#define VM_FLAG(name, v) name = v,
#include <j6/tables/vm_flags.inc>
#undef VM_FLAG
    driver_mask     = 0x000fffff, ///< flags allowed via syscall for drivers
    user_mask       = 0x0000ffff, ///< flags allowed via syscall for non-drivers
};
is_bitfield(vm_flags);


/// Virtual memory areas allow control over memory allocation
class vm_area :
    public kobject
{
public:
    /// Capabilities on a newly constructed vma handle
    constexpr static j6_cap_t creation_caps = j6_cap_vma_all;

    static constexpr kobject::type type = kobject::type::vma;

    /// Constructor.
    /// \arg size  Initial virtual size of the memory area
    /// \arg flags Flags for this memory area
    vm_area(size_t size, vm_flags flags = vm_flags::none);

    virtual ~vm_area();

    /// Get the current virtual size of the memory area
    inline size_t size() const { return m_size; }

    /// Get the flags set for this area
    inline vm_flags flags() const { return m_flags; }

    /// Track that this area was added to a vm_space
    /// \arg space  The space to add this area to
    /// \returns    False if this area cannot be added
    virtual bool add_to(vm_space *space);

    /// Track that this area was removed frm a vm_space
    /// \arg space  The space that is removing this area
    virtual void remove_from(vm_space *space);

    /// Change the virtual size of the memory area. This may cause
    /// deallocation if the new size is smaller than the current size.
    /// Note that if resizing is unsuccessful, the previous size will
    /// be returned.
    /// \arg size  The desired new virtual size
    /// \returns   The new virtual size
    virtual size_t resize(size_t size);

    /// Get the physical page for the given offset
    /// \arg offset The offset into the VMA
    /// \arg phys   [out] Receives the physical page address, if any
    /// \returns    True if there should be a page at the given offset
    virtual bool get_page(uintptr_t offset, uintptr_t &phys) = 0;

protected:
    /// A VMA is not deleted until both no handles remain AND it's not
    /// mapped by any VM space.
    virtual void on_no_handles() override;

    bool can_resize(size_t size);

    size_t m_size;
    vm_flags m_flags;
    util::vector<vm_space*> m_spaces;

    // Initial static space for m_spaces - most areas will never grow
    // beyond this size, so avoid allocations
    static constexpr size_t static_size = 2;
    vm_space *m_vector_static[static_size];
};


/// A shareable but non-allocatable memory area of contiguous physical
/// addresses (like mmio)
class vm_area_fixed :
    public vm_area
{
public:
    /// Constructor.
    /// \arg start Starting physical address of this area
    /// \arg size  Size of the physical memory area
    /// \arg flags Flags for this memory area
    vm_area_fixed(uintptr_t start, size_t size, vm_flags flags = vm_flags::none);
    virtual ~vm_area_fixed();

    virtual size_t resize(size_t size) override;
    virtual bool get_page(uintptr_t offset, uintptr_t &phys) override;

private:
    uintptr_t m_start;
};


/// Area that allows open allocation
class vm_area_open :
    public vm_area
{
public:
    /// Constructor.
    /// \arg size  Initial virtual size of the memory area
    /// \arg flags Flags for this memory area
    vm_area_open(size_t size, vm_flags flags);
    virtual ~vm_area_open();

    virtual bool get_page(uintptr_t offset, uintptr_t &phys) override;

private:
    page_tree *m_mapped;
};


/// Area that does not track its allocations and thus cannot be shared
class vm_area_untracked :
    public vm_area
{
public:
    /// Constructor.
    /// \arg size  Initial virtual size of the memory area
    /// \arg flags Flags for this memory area
    vm_area_untracked(size_t size, vm_flags flags);
    virtual ~vm_area_untracked();

    virtual bool add_to(vm_space *space) override;
    virtual bool get_page(uintptr_t offset, uintptr_t &phys) override;
};


/// Area split into standard-sized segments, separated by guard pages.
class vm_area_guarded :
    public vm_area_open
{
public:
    /// Constructor.
    /// \arg start     Initial address where this area is mapped
    /// \arg sec_pages Pages in an individual section
    /// \arg size      Initial virtual size of the memory area
    /// \arg flags     Flags for this memory area
    vm_area_guarded(
        uintptr_t start,
        size_t sec_pages,
        size_t size,
        vm_flags flags);

    virtual ~vm_area_guarded();

    /// Get an available section in this area
    uintptr_t get_section();

    /// Return a section address to the available pool
    void return_section(uintptr_t addr);

    virtual bool get_page(uintptr_t offset, uintptr_t &phys) override;

private:
    util::vector<uintptr_t> m_cache;
    uintptr_t m_start;
    size_t m_pages;
    uintptr_t m_next;
};

} // namespace obj
