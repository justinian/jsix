#pragma once
/// \file vm_area.h
/// Definition of VMA objects and related functions

#include "j6/signals.h"
#include "kutil/enum_bitfields.h"
#include "kutil/map.h"

#include "kernel_memory.h"
#include "objects/kobject.h"

class vm_space;

enum class vm_flags : uint32_t
{
	none            = 0x00000000,

	zero            = 0x00000001, 
	contiguous      = 0x00000002,

	large_pages     = 0x00000100,
	huge_pages      = 0x00000200
};

IS_BITFIELD(vm_flags);


/// Virtual memory areas allow control over memory allocation
class vm_area :
	public kobject
{
public:
	/// Constructor.
	/// \arg size  Initial virtual size of the memory area
	/// \arg flags Flags for this memory area
	vm_area(size_t size, vm_flags flags = vm_flags::none);
	virtual ~vm_area();

	static constexpr kobject::type type = kobject::type::vma;

	/// Get the current virtual size of the memory area
	size_t size() const { return m_size; }

	/// Change the virtual size of the memory area. This may cause
	/// deallocation if the new size is smaller than the current size.
	/// Note that if resizing is unsuccessful, the previous size will
	/// be returned.
	/// \arg size  The desired new virtual size
	/// \returns   The new virtual size
	size_t resize(size_t size);

	/// Add this virtual area to a process' virtual address space. If
	/// the given base address is zero, a base address will be chosen
	/// automatically.
	/// \arg s    The target address space
	/// \arg base The base address this area will be mapped to
	void add_to(vm_space *s, uintptr_t base);

	/// Remove this virtual area from a process' virtual address space.
	/// \arg s    The target address space
	void remove_from(vm_space *s);

	/// Commit contiguous physical pages to this area
	/// \arg phys   The physical address of the first page
	/// \arg offset The offset from the start of this area these pages represent
	/// \arg count  The number of pages
	/// \returns    True if successful
	bool commit(uintptr_t phys, uintptr_t offset, size_t count);

	/// Uncommit physical pages from this area
	/// \arg offset The offset from the start of this area these pages represent
	/// \arg count  The number of pages
	/// \returns    True if successful
	bool uncommit(uintptr_t offset, size_t count);

	/// Get the flags set for this area
	vm_flags flags() const { return m_flags; }

protected:
	virtual void on_no_handles() override;

private:
	struct mapping {
		uintptr_t offset;
		size_t count;
		uintptr_t phys;

		int compare(const struct mapping &o) const {
			return offset > o.offset ? 1 : offset < o.offset ? -1 : 0;
		}

		inline uintptr_t end() const { return offset + count * memory::frame_size; }
		inline uintptr_t phys_end() const { return phys + count * memory::frame_size; }
	};

	size_t overlaps(uintptr_t offset, size_t pages, size_t *count);

	void map(uintptr_t offset, size_t count, uintptr_t phys);
	void unmap(uintptr_t offset, size_t count);

	size_t m_size;
	vm_flags m_flags;
	kutil::map<vm_space*, uintptr_t> m_spaces;
	kutil::vector<mapping> m_mappings;
};
