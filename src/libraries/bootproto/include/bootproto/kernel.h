#pragma once
/// \file bootproto/kernel.h
/// Data structures for initializing the kernel

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include <util/bitset.h>
#include <util/counted.h>

namespace bootproto {

constexpr uint32_t args_magic = 'j6bp'; // "jsix boot protocol"
constexpr uint16_t args_version = 1;

constexpr uint64_t header_magic = 0x4c454e52454b366aull; // 'j6KERNEL'
constexpr uint16_t header_version = 2;
constexpr uint16_t min_header_version = 2;

enum class section_flags { none, execute, write, read };

struct program_section {
    uintptr_t phys_addr;
    uintptr_t virt_addr;
    uint32_t size;
    util::bitset32 type;
};

struct program {
    uintptr_t entrypoint;
    uintptr_t phys_base;
    util::counted<program_section> sections;
};

enum class mem_type : uint32_t {
    free,
    pending,
    acpi,
    uefi_runtime,
    mmio,
    persistent
};

/// Structure to hold an entry in the memory map.
struct mem_entry
{
    uintptr_t start;
    size_t pages;
    mem_type type;
    uint32_t attr;
};

enum class allocation_type : uint8_t {
    none, page_table, mem_map, frame_map, file, program, init_args,
};

/// A single contiguous allocation of pages
struct page_allocation
{
    uintptr_t address;
    uint32_t count;
    allocation_type type;
};

/// A page-sized register of page_allocation entries
struct allocation_register
{
    allocation_register *next;
    uint8_t count;

    uint8_t reserved0;
    uint16_t reserved1;
    uint32_t reserved2;

    page_allocation entries[255];
};

enum class frame_flags : uint32_t {
    uncacheable         = 0x00000001,
    write_combining     = 0x00000002,
    write_through       = 0x00000004,
    write_back          = 0x00000008,
    uncache_exported    = 0x00000010,

    write_protect       = 0x00001000,
    read_protect        = 0x00002000,
    exec_protect        = 0x00004000,
    non_volatile        = 0x00008000,

    read_only           = 0x00020000,
    earmarked           = 0x00040000,
    hw_crypto           = 0x00080000,
};


constexpr size_t frames_per_block = 64 * 64 * 64;

struct frame_block
{
    uintptr_t base;
    uint32_t count;
    frame_flags flags;
    uint64_t map1;
    uint64_t map2[64];
    uint64_t *bitmap;
};

enum class boot_flags { none, debug, test };

struct args
{
    uint32_t magic;
    uint16_t version;
    util::bitset16 flags;

    void *pml4;
    util::counted<void> page_tables;
    util::counted<mem_entry> mem_map;
    util::counted<frame_block> frame_blocks;
    allocation_register *allocations;

    uintptr_t panic_handler;
    util::buffer symbol_table;

    program init;
    uintptr_t init_modules;

    void *runtime_services;
    void *acpi_table;
}
__attribute__((aligned(alignof(max_align_t))));

struct header
{
    uint64_t magic;

    uint16_t length;
    uint16_t version;

    uint16_t version_major;
    uint16_t version_minor;
    uint16_t version_patch;
    uint16_t reserved;

    uint32_t version_gitsha;

    uint64_t flags;
};

using entrypoint = __attribute__((sysv_abi)) void (*)(args *);

} // namespace bootproto
