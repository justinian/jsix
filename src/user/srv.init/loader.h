#pragma once
/// \file loader.h
/// Routines for loading and starting other programs

namespace kernel {
namespace init {
    struct module_program;
}}

bool load_program(const kernel::init::module_program &prog, char *err_msg);
