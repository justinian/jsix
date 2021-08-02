/// \file loader.h
/// Definitions for loading the kernel into memory
#pragma once

#include "counted.h"

namespace kernel {
namespace init {
    struct program;
    struct module;
}}

namespace boot {

namespace fs {
    class file;
}

namespace loader {

struct program_desc
{
    const wchar_t *name;
    const wchar_t *path;
};

/// Load a file from disk into memory.
/// \arg disk  The opened UEFI filesystem to load from
/// \arg desc  The program descriptor identifying the file
buffer
load_file(
    fs::file &disk,
    const program_desc &desc);

/// Parse and load an ELF file in memory into a loaded image.
/// \arg disk       The opened UEFI filesystem to load from
/// \arg desc       The program descriptor identifying the program
/// \arg add_module Also create a module for this loaded program
kernel::init::program *
load_program(
    fs::file &disk,
    const program_desc &desc,
    bool add_module = false);

/// Load a file from disk into memory, creating an init args module
/// \arg disk  The opened UEFI filesystem to load from
/// \arg desc  The program descriptor identifying the file
void
load_module(
    fs::file &disk,
    const program_desc &desc);

/// Verify that a loaded ELF has the j6 kernel header
/// \arg program  The program to check for a header
void
verify_kernel_header(kernel::init::program &program);

} // namespace loader
} // namespace boot
