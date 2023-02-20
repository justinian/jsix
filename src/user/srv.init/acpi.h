#pragma once
/// \file acpi.h
/// Routines for loading and parsing ACPI tables

#include <util/counted.h>
#include <j6/types.h>

namespace bootproto {
    struct module;
}

void load_acpi(j6_handle_t sys, const bootproto::module *mod);
