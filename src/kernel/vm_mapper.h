#pragma once
/// \file vm_mapper.h
/// VMA to address space mapping interface and implementing objects

#include <stdint.h>
#include "kutil/vector.h"

class vm_area;
class vm_space;

/// An interface to map vm_areas to one or more vm_spaces
class vm_mapper
{
public:
	virtual ~vm_mapper() {}

	/// Check whether the owning VMA can be resized to the given size.
	/// \arg size The desired size
	/// \returns  True if resize is possible
	virtual bool can_resize(size_t size) const = 0;

	/// Map the given physical pages into the owning VMA at the given offset
	/// \arg offset Offset into the VMA of the requested virtual address
	/// \arg count  Number of contiguous physical pages to map
	/// \arg phys   The starting physical address of the pages
	virtual void map(uintptr_t offset, size_t count, uintptr_t phys) = 0;

	/// Unmap the pages corresponding to the given offset from the owning VMA
	/// \arg offset Offset into the VMA of the requested virtual address
	/// \arg count  Number of pages to unmap
	virtual void unmap(uintptr_t offset, size_t count) = 0;

	/// Add the given address space to the list of spaces the owning VMA is
	/// mapped to, if applicable.
	virtual void add(vm_space *space) {}

	/// Remove the given address space from the list of spaces the owning VMA
	/// is mapped to, if applicable.
	virtual void remove(vm_space *space) {}
};


/// A vm_mapper that maps a VMA to a single vm_space
class vm_mapper_single :
	public vm_mapper
{
public:
	vm_mapper_single(vm_area &area, vm_space &space);
	virtual ~vm_mapper_single();

	virtual bool can_resize(size_t size) const override;
	virtual void map(uintptr_t offset, size_t count, uintptr_t phys) override;
	virtual void unmap(uintptr_t offset, size_t count) override;

	vm_space & space() { return m_space; }

	virtual void remove(vm_space *space) override;

private:
	vm_area &m_area;
	vm_space &m_space;
};


/// A vm_mapper that maps a VMA to multiple vm_spaces
class vm_mapper_multi :
	public vm_mapper
{
public:
	vm_mapper_multi(vm_area &area);
	virtual ~vm_mapper_multi();

	virtual bool can_resize(size_t size) const override;
	virtual void map(uintptr_t offset, size_t count, uintptr_t phys) override;
	virtual void unmap(uintptr_t offset, size_t count) override;
	virtual void add(vm_space *space) override;
	virtual void remove(vm_space *space) override;

private:
	vm_area &m_area;
	kutil::vector<vm_space*> m_spaces;
};

