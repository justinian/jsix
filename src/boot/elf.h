/// \file elf.h
/// Definitions and related constants for ELF64 structures
#pragma once
#include <stdint.h>

namespace boot {
namespace elf {

constexpr uint8_t version = 1;
constexpr uint8_t word_size = 2;
constexpr uint8_t endianness = 1;
constexpr uint8_t os_abi = 0;
constexpr uint16_t machine = 0x3e;

const unsigned PT_LOAD = 1;
const unsigned ST_PROGBITS = 1;
const unsigned ST_NOBITS = 8;
const unsigned long SHF_ALLOC = 0x2;

struct header
{
	char magic[4];

	uint8_t word_size;
	uint8_t endianness;
	uint8_t header_version;
	uint8_t os_abi;

	uint64_t reserved;

	uint16_t type;
	uint16_t machine;

	uint32_t version;

	uint64_t entrypoint;
	uint64_t ph_offset;
	uint64_t sh_offset;

	uint32_t flags;

	uint16_t eh_size;

	uint16_t ph_entsize;
	uint16_t ph_num;

	uint16_t sh_entsize;
	uint16_t sh_num;

	uint16_t sh_str_idx;
} __attribute__ ((packed));

struct program_header
{
	uint32_t type;
	uint32_t flags;
	uint64_t offset;

	uint64_t vaddr;
	uint64_t paddr;

	uint64_t file_size;
	uint64_t mem_size;

	uint64_t align;
} __attribute__ ((packed));

struct section_header
{
	uint32_t name;
	uint32_t type;
	uint64_t flags;
	uint64_t addr;
	uint64_t offset;
	uint64_t size;
	uint32_t link;
	uint32_t info;
	uint64_t align;
	uint64_t entry_size;
} __attribute__ ((packed));

} // namespace elf
} // namespace boot
