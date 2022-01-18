#pragma once
/// \file event.h
/// Definition of event kobject types

#include "objects/kobject.h"

namespace obj {

class event :
    public kobject
{
public:
    /// Capabilities on a newly constructed event handle
    constexpr static j6_cap_t creation_caps = 0;

    event() :
        kobject(type::event) {}

    static constexpr kobject::type type = kobject::type::event;
};

} // namespace obj
