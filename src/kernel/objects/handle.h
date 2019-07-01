#pragma once
/// \file handle.h
/// Defines types for user handles to kernel objects

#include "j6/types.h"
#include "objects/kobject.h"

class handle
{
public:
	/// Move constructor. Takes ownership of the object from other.
	handle(handle&& other);

	/// Constructor.
	/// \arg owner   koid of the process that has this handle
	/// \arg rights  access rights this handle has over the object
	/// \arg obj     the object held
	handle(j6_koid_t owner, j6_rights_t rights, kobject *obj);

	~handle();

	handle() = delete;
	handle(const handle &other) = delete;
	handle & operator=(const handle& other) = delete;

private:
	j6_koid_t m_owner;
	kobject *m_object;
	j6_rights_t m_rights;
};
