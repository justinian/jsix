#include "log.h"
#include "objects/thread.h"
#include "objects/vm_area.h"
#include "page_manager.h"
#include "vm_space.h"

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


vm_space::vm_space(page_table *p) : m_kernel(true), m_pml4(p) {}

vm_space::vm_space() :
	m_kernel(false)
{
	m_pml4 = page_table::get_table_page();
	page_table *kpml4 = kernel_space().m_pml4;

	kutil::memset(m_pml4, 0, memory::frame_size/2);
	for (unsigned i = memory::pml4e_kernel; i < memory::table_entries; ++i)
		m_pml4->entries[i] = kpml4->entries[i];
}

vm_space::~vm_space()
{
	for (auto &a : m_areas)
		a.area->remove_from(this);

	kassert(!is_kernel(), "Kernel vm_space destructor!");

	vm_space &kernel = kernel_space();

	if (active())
		kernel.activate();

	// All VMAs have been removed by now, so just
	// free all remaining pages and tables
	m_pml4->free(page_table::level::pml4);
}

vm_space &
vm_space::kernel_space()
{
	extern vm_space &g_kernel_space;
	return g_kernel_space;
}

bool
vm_space::add(uintptr_t base, vm_area *area)
{
	//TODO: check for collisions
	m_areas.sorted_insert({base, area});
	return true;
}

bool
vm_space::remove(vm_area *area)
{
	for (auto &a : m_areas) {
		if (a.area == area) {
			m_areas.remove(a);
			return true;
		}
	}
	return false;
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

void
vm_space::page_in(uintptr_t addr, size_t count, uintptr_t phys)
{
	page_manager *pm = page_manager::get();
	pm->page_in(m_pml4, phys, addr, count, is_kernel());
}

void
vm_space::page_out(uintptr_t addr, size_t count)
{
	page_manager *pm = page_manager::get();
	pm->page_out(m_pml4, addr, count, false);
}

void
vm_space::allow(uintptr_t start, size_t length, bool allow)
{
	using level = page_table::level;
	kassert((start & 0xfff) == 0, "non-page-aligned address");
	kassert((length & 0xfff) == 0, "non-page-aligned length");

	const uintptr_t end = start + length;
	page_table::iterator it {start, m_pml4};

	while (it.vaddress() < end) {
		level d = it.align();
		while (it.end(d) > end) ++d;
		it.allow(d-1, allow);
		it.next(d);
	}
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
	uintptr_t page = addr & ~0xfffull;

	page_table::iterator it {addr, m_pml4};

	if (!it.allowed())
		return false;

	// TODO: pull this out of PM
	page_manager::get()->map_pages(page, 1, m_pml4);

	/* TODO: Tell the VMA if there is one
	uintptr_t base = 0;
	vm_area *area = get(addr, &base);
	*/

	return true;
}

size_t
vm_space::copy(vm_space &source, vm_space &dest, void *from, void *to, size_t length)
{
	uintptr_t ifrom = reinterpret_cast<uintptr_t>(from);
	uintptr_t ito = reinterpret_cast<uintptr_t>(to);

	page_table::iterator sit {ifrom, source.m_pml4};
	page_table::iterator dit {ito, dest.m_pml4};

	// TODO: iterate page mappings and continue copying. For now i'm blindly
	// assuming both buffers are fully contained within single pages
	kutil::memcpy(
		memory::to_virtual<void>((*dit & ~0xfffull) | (ito & 0xffful)),
		memory::to_virtual<void>((*sit & ~0xfffull) | (ifrom & 0xffful)),
		length);

	return length;
}
