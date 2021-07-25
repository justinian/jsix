#pragma once
/// \file allocator.h
/// Page allocator class definition

#include "kernel_args.h"

namespace uefi {
	class boot_services;
}

namespace boot {
namespace memory {

using alloc_type = kernel::init::allocation_type;

class allocator
{
public:
	using allocation_register = kernel::init::allocation_register;

	allocator(uefi::boot_services &bs);

	void * allocate(size_t size, bool zero = false);
	void free(void *p);

	void * allocate_pages(size_t count, alloc_type type, bool zero = false);

	void memset(void *start, size_t size, uint8_t value);
	void copy(void *to, void *from, size_t size);

	inline void zero(void *start, size_t size) { memset(start, size, 0); }

	allocation_register * get_register() { return m_register; }

private:
	void add_register();

	uefi::boot_services &m_bs;

	allocation_register *m_register;
	allocation_register *m_current;
};

/// Initialize the global allocator
void init_allocator(uefi::boot_services *bs);

} // namespace memory

extern memory::allocator &g_alloc;

} // namespace boot

void * operator new (size_t size, void *p);
void * operator new(size_t size);
void * operator new [] (size_t size);
void operator delete (void *p) noexcept;
void operator delete [] (void *p) noexcept;
