/// \file loader.h
/// Definitions for loading the kernel into memory
#pragma once

#include <util/counted.h>

namespace bootproto {
    struct program;
    struct module;
}

namespace boot {

class descriptor;

namespace fs {
    class file;
}

namespace loader {

/// Load a file from disk into memory.
/// \arg disk  The opened UEFI filesystem to load from
/// \arg desc  The program descriptor identifying the file
util::buffer
load_file(
    fs::file &disk,
    const descriptor &desc);

/// Parse and load an ELF file in memory into a loaded image.
/// \arg disk       The opened UEFI filesystem to load from
/// \arg desc       The descriptor identifying the program
/// \arg add_module Also create a module for this loaded program
bootproto::program *
load_program(
    fs::file &disk,
    const descriptor &desc,
    bool add_module = false);

/// Load a file from disk into memory, creating an init args module
/// \arg disk  The opened UEFI filesystem to load from
/// \arg desc  The program descriptor identifying the file
void
load_module(
    fs::file &disk,
    const descriptor &desc);

/// Verify that a loaded ELF has the j6 kernel header
/// \arg program  The program to check for a header
void
verify_kernel_header(bootproto::program &program);

} // namespace loader
} // namespace boot
