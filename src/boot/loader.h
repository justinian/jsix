/// \file loader.h
/// Definitions for loading the kernel into memory
#pragma once

#include <bootproto/init.h>
#include <bootproto/kernel.h>
#include <util/counted.h>

namespace bootproto {
    struct program;
}

namespace boot {

namespace fs { class file; }
namespace paging { class pager; }

namespace loader {

// Bootloader ELF file requirements
// ================================
// The bootloader accepts a subset of valid ELF files to load, with
// the following requiresments:
//  1. All program segments are page-aligned.
//  2. PT_LOAD segments cannot contain a mix of PROGBITS and NOBITS
//     sections. i.e., section memory size must equal either zero or
//     its file size.
//  3. There are only one or zero PT_LOAD NOBITS program segments.


/// Load a file from disk into memory.
/// \arg disk  The opened UEFI filesystem to load from
/// \arg path  The path of the file to load
util::buffer load_file(fs::file &disk, const wchar_t *path);

/// Parse a buffer holding ELF data into a bootproto::program
/// \arg name   The human-readable name of the program to load
/// \arg data    A buffer containing an ELF executable
/// \arg program A program structure to fill
void parse_program(
    const wchar_t *name,
    util::const_buffer data,
    bootproto::program &program);

/// Parse a buffer holding ELF data and map it to be runnable
/// \arg data   The ELF data in memory
/// \arg name   The human-readable name of the program to load
/// \arg pager  The kernel space pager, to map programs into
/// \arg verify If this is the kernel and should have its header verified
/// \returns    The entrypoint to the loaded program
uintptr_t
load_program(
    util::const_buffer data,
    const wchar_t *name,
    paging::pager &pager,
    bool verify = false);

/// Load a file from disk into memory, creating an init args module
/// \arg disk    The opened UEFI filesystem to load from
/// \arg name    The human-readable name of the module
/// \arg path    The path of the file to load the module from
/// \arg type    The major type to set on the module
void
load_module(
    fs::file &disk,
    const wchar_t *name,
    const wchar_t *path,
    bootproto::module_type type);

} // namespace loader
} // namespace boot
