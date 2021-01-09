#pragma once

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

namespace kernel {
namespace args {

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

struct program {
	uintptr_t phys_addr;
	uintptr_t virt_addr;
	uintptr_t entrypoint;
	size_t size;
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

enum class boot_flags : uint16_t {
	none  = 0x0000,
	debug = 0x0001
};

struct header {
	uint32_t magic;
	uint16_t version;
	boot_flags flags;

	void *pml4;
	void *page_tables;
	size_t table_count;

	program *programs;
	size_t num_programs;

	module *modules;
	size_t num_modules;

	mem_entry *mem_map;
	size_t map_count;

	void *runtime_services;
	void *acpi_table;

	framebuffer video;
}
__attribute__((aligned(alignof(max_align_t))));

} // namespace args

using entrypoint = __attribute__((sysv_abi)) void (*)(args::header *);

} // namespace kernel
