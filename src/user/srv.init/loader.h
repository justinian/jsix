#pragma once
/// \file loader.h
/// Routines for loading and starting other programs

#include <j6/types.h>

namespace bootproto {
    struct module_program;
}

bool load_program(
        const char *name,
        util::const_buffer data,
        j6_handle_t sys, j6_handle_t slp,
        char *err_msg);

j6_handle_t map_phys(j6_handle_t sys, uintptr_t phys, size_t len, uintptr_t addr = 0);

inline j6_handle_t map_phys(j6_handle_t sys, const void *phys, size_t len, uintptr_t addr = 0) {
    return map_phys(sys, reinterpret_cast<uintptr_t>(phys), len, addr);
}
