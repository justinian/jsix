#pragma once

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

namespace kernel {
namespace init {

constexpr uint32_t magic = 0x600dda7a;
constexpr uint16_t version = 1;

enum class mod_type : uint32_t {
	symbol_table
};

struct module {
	void *location;
	size_t size;
	mod_type type;
};

enum class fb_type : uint16_t {
	none,
	rgb8,
	bgr8
};

struct framebuffer {
	uintptr_t phys_addr;
	size_t size;
	uint32_t vertical;
	uint32_t horizontal;
	uint16_t scanline;
	fb_type type;
};

enum class section_flags : uint32_t {
	none    = 0,
	execute = 1,
	write   = 2,
	read    = 4,
};

struct program_section {
	uintptr_t phys_addr;
	uintptr_t virt_addr;
	uint32_t size;
	section_flags type;
};

struct program {
	uintptr_t entrypoint;
	uintptr_t base;
	size_t total_size;
	size_t num_sections;
	program_section sections[3];
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

constexpr size_t frames_per_block = 64 * 64 * 64;

struct frame_block
{
	uintptr_t base;
	uint32_t count;
	uint32_t attrs;
	uint64_t map1;
	uint64_t map2[64];
	uint64_t *bitmap;
};

enum class boot_flags : uint16_t {
	none  = 0x0000,
	debug = 0x0001
};

struct args {
	uint32_t magic;
	uint16_t version;
	boot_flags flags;

	void *pml4;
	void *page_tables;
	size_t table_count;
	size_t table_pages;

	program *programs;
	size_t num_programs;

	module *modules;
	size_t num_modules;

	mem_entry *mem_map;
	size_t map_count;

	frame_block *frame_blocks;
	size_t frame_block_count;
	size_t frame_block_pages;

	void *runtime_services;
	void *acpi_table;

	framebuffer video;
}
__attribute__((aligned(alignof(max_align_t))));

} // namespace init

using entrypoint = __attribute__((sysv_abi)) void (*)(init::args *);

} // namespace kernel
