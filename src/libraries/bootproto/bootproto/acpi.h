#pragma once
/// \file bootproto/acpi.h
/// Data structures for passing ACPI tables to the init server

#include <util/counted.h>

namespace bootproto {

struct acpi
{
    util::const_buffer region;
    void const *root;
};

} // namespace bootproto
