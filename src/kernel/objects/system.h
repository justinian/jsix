#pragma once
/// \file system.h
/// Definition of kobject type representing the system

#include <j6/caps.h>
#include "objects/kobject.h"

namespace obj {

class system :
    public kobject
{
public:
    /// Capabilities on system given to init
    constexpr static j6_cap_t init_caps = j6_cap_system_all;

    static constexpr kobject::type type = kobject::type::system;

    inline static system & get() { return s_instance; }

private:
    static system s_instance;
    system() : kobject(type::system) {}
};

} // namespace obj
