#pragma once
/// \file event.h
/// Definition of event kobject types

#include "objects/kobject.h"

class event :
    public kobject
{
public:
    event() :
        kobject(type::event) {}

    static constexpr kobject::type type = kobject::type::event;
};
