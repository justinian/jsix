#pragma once
/// \file loader.h
/// Routines for loading and starting other programs

#include <j6/types.h>

namespace bootproto {
    struct module_program;
}

bool load_program(
        const bootproto::module_program &prog,
        j6_handle_t sys, j6_handle_t slp,
        char *err_msg);
