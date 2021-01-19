#pragma once
/// \file system.h
/// Definition of kobject type representing the system

#include "objects/kobject.h"

class system :
	public kobject
{
public:
	static constexpr kobject::type type = kobject::type::event;

	inline static system & get() { return s_instance; }

private:
	static system s_instance;
	system() : kobject(type::system) {}
};
