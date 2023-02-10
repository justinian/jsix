#pragma once
/// \file modules.h
/// Routines for loading initial argument modules

#include <vector>

#include <bootproto/init.h>
#include <j6/types.h>
#include <util/pointers.h>

void
load_modules(
        uintptr_t address,
        j6_handle_t system,
        j6_handle_t self,
        std::vector<const bootproto::module *> &modules);
