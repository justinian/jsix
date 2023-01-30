/// \file loader.h
/// Definitions for loading the kernel into memory
#pragma once

#include <bootproto/init.h>
#include <util/counted.h>

namespace bootproto {
    struct program;
}

namespace boot {

class descriptor;

namespace fs {
    class file;
}

namespace loader {

/// Load a file from disk into memory.
/// \arg disk  The opened UEFI filesystem to load from
/// \arg path  The path of the file to load
util::buffer
load_file(
    fs::file &disk,
    const wchar_t *path);

/// Parse and load an ELF file in memory into a loaded image.
/// \arg disk   The opened UEFI filesystem to load from
/// \arg desc   The descriptor identifying the program
/// \arg name   The human-readable name of the program to load
/// \arg verify If this is the kernel and should have its header verified
bootproto::program *
load_program(
    fs::file &disk,
    const wchar_t *name,
    const descriptor &desc,
    bool verify = false);

/// Load a file from disk into memory, creating an init args module
/// \arg disk    The opened UEFI filesystem to load from
/// \arg name    The human-readable name of the module
/// \arg path    The path of the file to load the module from
/// \arg type    The major type to set on the module
/// \arg subtype The subtype to set on the module
void
load_module(
    fs::file &disk,
    const wchar_t *name,
    const wchar_t *path,
    bootproto::module_type type,
    uint16_t subtype);

} // namespace loader
} // namespace boot
