#pragma once
/// \file vm_space.h
/// Structure for tracking a range of virtual memory addresses

#include <stdint.h>
#include "kutil/enum_bitfields.h"
#include "kutil/vector.h"

struct page_table;
class process;
struct TCB;
class vm_area;

/// Tracks a region of virtual memory address space
class vm_space
{
public:
	/// Constructor for the kernel address space
	/// \arg pml4   The existing kernel PML4
	vm_space(page_table *pml4);

	/// Constructor. Creates a new address space.
	vm_space();

	~vm_space();

	/// Add a virtual memorty area to this address space
	/// \arg base  The starting address of the area
	/// \arg area  The area to add
	/// \returns   True if the add succeeded
	bool add(uintptr_t base, vm_area *area);

	/// Remove a virtual memory area from this address space
	/// \arg area  The area to remove
	/// \returns   True if the area was removed
	bool remove(vm_area *area);

	/// Get the virtual memory area corresponding to an address
	/// \arg addr  The address to check
	/// \arg base  [out] if not null, receives the base address of the area
	/// \returns   The vm_area, or nullptr if not found
	vm_area * get(uintptr_t addr, uintptr_t *base = nullptr);

	/// Check if this is the kernel space
	inline bool is_kernel() const { return m_kernel; }

	/// Get the kernel virtual memory space
	static vm_space & kernel_space();

	/// Copy a range of mappings from the given address space
	/// \arg virt  The starting virutal address
	/// \arg phys  The starting physical address
	/// \arg count The number of contiugous physical pages to map
	void page_in(uintptr_t virt, uintptr_t phys, size_t count);

	/// Clear mappings from the given region
	/// \arg start  The starting virutal address to clear
	/// \arg count  The number of pages worth of mappings to clear
	void clear(uintptr_t start, size_t count);

	/// Mark whether allocation is allowed or not in a range of
	/// virtual memory.
	/// \arg start  The starting virtual address of the area
	/// \arg length The length in bytes of the area
	/// \arg allow  True if allocation should be allowed
	void allow(uintptr_t start, size_t length, bool allow);

	/// Check if this space is the current active space
	bool active() const;

	/// Set this space as the current active space
	void activate() const;

	enum class fault_type : uint8_t {
		none     = 0x00,
		present  = 0x01,
		write    = 0x02,
		user     = 0x04,
		reserved = 0x08,
		fetch    = 0x10
	};

	/// Handle a page fault.
	/// \arg addr  Address which caused the fault
	/// \arg ft    Flags from the interrupt about the kind of fault
	/// \returns   True if the fault was successfully handled
	bool handle_fault(uintptr_t addr, fault_type fault);

	/// Set up a TCB to operate in this address space.
	void initialize_tcb(TCB &tcb);

	/// Copy data from one address space to another
	/// \arg source The address space data is being copied from
	/// \arg dest   The address space data is being copied to
	/// \arg from   Pointer to the data in the source address space
	/// \arg to     Pointer to the destination in the dest address space
	/// \arg length Amount of data to copy, in bytes
	/// \returnd    The number of bytes copied
	static size_t copy(vm_space &source, vm_space &dest, void *from, void *to, size_t length);

private:
	bool m_kernel;
	page_table *m_pml4;

	struct area {
		uintptr_t base;
		vm_area *area;
		int compare(const struct area &o) const;
		bool operator==(const struct area &o) const;
	};
	kutil::vector<area> m_areas;
};

IS_BITFIELD(vm_space::fault_type);
