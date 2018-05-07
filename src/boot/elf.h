#pragma once
#include <stdint.h>

#ifndef ELF_VERSION
#define ELF_VERSION 1
#endif

#ifndef ELF_WORDSIZE
#define ELF_WORDSIZE 2
#endif

#ifndef ELF_ENDIAN
#define ELF_ENDIAN 1
#endif

#ifndef ELF_OSABI
#define ELF_OSABI 0
#endif

#ifndef ELF_MACHINE
#define ELF_MACHINE 0x3e
#endif

const unsigned ELF_PT_LOAD = 1;
const unsigned ELF_ST_PROGBITS = 1;
const unsigned ELF_ST_NOBITS = 8;
const unsigned long ELF_SHF_ALLOC = 0x2;

struct elf_ident
{
	char magic[4];

	uint8_t word_size;
	uint8_t endianness;
	uint8_t version;
	uint8_t os_abi;

	uint64_t reserved;
} __attribute__ ((packed));

struct elf_header
{
	struct elf_ident ident;

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

struct elf_program_header
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

struct elf_section_header
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
