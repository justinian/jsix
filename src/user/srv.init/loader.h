#pragma once
/// \file loader.h
/// Routines for loading and starting other programs

namespace bootproto {
    struct module_program;
}

bool load_program(const bootproto::module_program &prog, char *err_msg);
