#include "objects/handle.h"

handle::handle(handle&& other) :
	m_owner(other.m_owner),
	m_object(other.m_object),
	m_rights(other.m_rights)
{
	other.m_owner = 0;
	other.m_object = nullptr;
	other.m_rights = 0;
}

handle::handle(j6_koid_t owner, j6_rights_t rights, kobject *obj) :
	m_owner(owner),
	m_object(obj),
	m_rights(rights)
{
	if (m_object)
		m_object->handle_retain();
}

handle::~handle()
{
	if (m_object)
		m_object->handle_release();
}
