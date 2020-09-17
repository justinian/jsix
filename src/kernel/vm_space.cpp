#include "log.h"
#include "objects/process.h"
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


vm_space::vm_space(page_table *p, bool kernel) :
	m_kernel(kernel),
	m_pml4(p)
{
}

vm_space::~vm_space()
{
	for (auto &a : m_areas)
		a.area->remove_from(this);
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
