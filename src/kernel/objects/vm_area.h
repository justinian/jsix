#pragma once
/// \file vm_area.h
/// Definition of VMA objects and related functions

#include <stddef.h>
#include <stdint.h>

#include "j6/signals.h"
#include "kutil/enum_bitfields.h"
#include "kutil/vector.h"

#include "kernel_memory.h"
#include "objects/kobject.h"
#include "vm_mapper.h"

class vm_space;

enum class vm_flags : uint32_t
{
	none            = 0x00000000,

	write           = 0x00000001,
	exec            = 0x00000002,

	zero            = 0x00000010, 
	contiguous      = 0x00000020,

	large_pages     = 0x00000100,
	huge_pages      = 0x00000200,

	user_mask       = 0x0000ffff  ///< flags allowed via syscall
};


/// Virtual memory areas allow control over memory allocation
class vm_area :
	public kobject
{
public:
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

	/// Change the virtual size of the memory area. This may cause
	/// deallocation if the new size is smaller than the current size.
	/// Note that if resizing is unsuccessful, the previous size will
	/// be returned.
	/// \arg size  The desired new virtual size
	/// \returns   The new virtual size
	size_t resize(size_t size);

	/// Get the mapper object that maps this area to address spaces
	virtual vm_mapper & mapper() = 0;
	virtual const vm_mapper & mapper() const = 0;

	/// Check whether allocation at the given offset is allowed
	virtual bool allowed(uintptr_t offset) const { return true; }

	/// Commit contiguous physical pages to this area
	/// \arg phys   The physical address of the first page
	/// \arg offset The offset from the start of this area these pages represent
	/// \arg count  The number of pages
	virtual void commit(uintptr_t phys, uintptr_t offset, size_t count);

	/// Uncommit physical pages from this area
	/// \arg offset The offset from the start of this area these pages represent
	/// \arg count  The number of pages
	virtual void uncommit(uintptr_t offset, size_t count);

protected:
	virtual void on_no_handles() override;

	size_t m_size;
	vm_flags m_flags;
};


/// The standard, sharable, user-controllable VMA type
class vm_area_shared :
	public vm_area
{
public:
	/// Constructor.
	/// \arg size  Initial virtual size of the memory area
	/// \arg flags Flags for this memory area
	vm_area_shared(size_t size, vm_flags flags = vm_flags::none);
	virtual ~vm_area_shared();

	virtual vm_mapper & mapper() override { return m_mapper; }
	virtual const vm_mapper & mapper() const override { return m_mapper; }

private:
	vm_mapper_multi m_mapper;
};


/// Area that allows open allocation (eg, kernel heap)
class vm_area_open :
	public vm_area
{
public:
	/// Constructor.
	/// \arg size  Initial virtual size of the memory area
	/// \arg space The address space this area belongs to
	/// \arg flags Flags for this memory area
	vm_area_open(size_t size, vm_space &space, vm_flags flags);

	virtual vm_mapper & mapper() override { return m_mapper; }
	virtual const vm_mapper & mapper() const override { return m_mapper; }

	virtual void commit(uintptr_t phys, uintptr_t offset, size_t count) override;
	virtual void uncommit(uintptr_t offset, size_t count) override;

private:
	vm_mapper_single m_mapper;
};


/// Area split into standard-sized segments
class vm_area_buffers :
	public vm_area
{
public:
	/// Constructor.
	/// \arg size      Initial virtual size of the memory area
	/// \arg space     The address space this area belongs to
	/// \arg flags     Flags for this memory area
	/// \arg buf_pages Pages in an individual buffer
	vm_area_buffers(
		size_t size,
		vm_space &space,
		vm_flags flags,
		size_t buf_pages);

	/// Get an available stack address
	uintptr_t get_buffer();

	/// Return a buffer address to the available pool
	void return_buffer(uintptr_t addr);

	virtual vm_mapper & mapper() override { return m_mapper; }
	virtual const vm_mapper & mapper() const override { return m_mapper; }

	virtual bool allowed(uintptr_t offset) const override;
	virtual void commit(uintptr_t phys, uintptr_t offset, size_t count) override;
	virtual void uncommit(uintptr_t offset, size_t count) override;

private:
	vm_mapper_single m_mapper;
	kutil::vector<uintptr_t> m_cache;
	size_t m_pages;
	uintptr_t m_next;
};


/// Area backed by an external source (like a loaded program)
class vm_area_backed :
	public vm_area
{
public:
	/// Constructor.
	/// \arg size  Initial virtual size of the memory area
	/// \arg flags Flags for this memory area
	vm_area_backed(size_t size, vm_flags flags);

	virtual vm_mapper & mapper() override { return m_mapper; }
	virtual const vm_mapper & mapper() const override { return m_mapper; }

	virtual void commit(uintptr_t phys, uintptr_t offset, size_t count) override;
	virtual void uncommit(uintptr_t offset, size_t count) override;

private:
	vm_mapper_multi m_mapper;
};

IS_BITFIELD(vm_flags);
