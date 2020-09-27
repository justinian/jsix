#include "objects/vm_area.h"
#include "vm_mapper.h"
#include "vm_space.h"


vm_mapper_single::vm_mapper_single(vm_area &area, vm_space &space) :
	m_area(area), m_space(space)
{}

vm_mapper_single::~vm_mapper_single()
{
	m_space.clear(m_area, 0, memory::page_count(m_area.size()), true);
}

bool
vm_mapper_single::can_resize(size_t size) const
{
	return m_space.can_resize(m_area, size);
}

void
vm_mapper_single::map(uintptr_t offset, size_t count, uintptr_t phys)
{
	m_space.page_in(m_area, offset, phys, count);
}

void
vm_mapper_single::unmap(uintptr_t offset, size_t count)
{
	m_space.clear(m_area, offset, count, true);
}



vm_mapper_multi::vm_mapper_multi(vm_area &area) :
	m_area(area)
{
}

vm_mapper_multi::~vm_mapper_multi()
{
	if (!m_spaces.count())
		return;

	size_t count = memory::page_count(m_area.size());

	for (int i = 1; i < m_spaces.count(); ++i)
		m_spaces[i]->clear(m_area, 0, count);

	m_spaces[0]->clear(m_area, 0, count, true);
}

bool
vm_mapper_multi::can_resize(size_t size) const
{
	for (auto &it : m_spaces)
		if (!it->can_resize(m_area, size))
			return false;
	return true;
}

void
vm_mapper_multi::map(uintptr_t offset, size_t count, uintptr_t phys)
{
	for (auto &it : m_spaces)
		it->page_in(m_area, offset, phys, count);
}

void
vm_mapper_multi::unmap(uintptr_t offset, size_t count)
{
	for (auto &it : m_spaces)
		it->clear(m_area, offset, count);
}

void
vm_mapper_multi::add(vm_space *space)
{
	if (m_spaces.count()) {
		vm_space *source = m_spaces[0];
		space->copy_from(*source, m_area);
	}

	m_spaces.append(space);
}

void
vm_mapper_multi::remove(vm_space *space)
{
	size_t count = memory::page_count(m_area.size());

	for (int i = 0; i < m_spaces.count(); ++i) {
		if (m_spaces[i] == space) {
			m_spaces.remove_swap_at(i);
			space->clear(m_area, 0, count, m_spaces.count() == 0);
		}
	}
}
