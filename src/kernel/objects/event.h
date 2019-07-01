#pragma once
/// \file event.h
/// Definition of event kobject types

#include "objects/kobject.h"

class event :
	public kobject
{
public:
	static constexpr type type_id = type::event;

	event() :
		kobject(type_id) {}
};
