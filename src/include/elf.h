#pragma once

/* elf.h - basic defines for external code written assuming <elf.h> works. Only
 * Elf64 values are included.
 */

typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Section;
typedef Elf64_Half Elf64_Versym;

typedef struct {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
} Elf64_Rel;

typedef struct {
	Elf64_Addr r_offset;
	Elf64_Word r_info;
	Elf64_Sword r_addend;
} Elf64_Rela;

typedef struct {
	Elf64_Sxword d_tag;
	union {
		Elf64_Xword d_val;
		Elf64_Addr d_ptr;
	} d_un;
} Elf64_Dyn;

#define ELF64_R_TYPE(x) ((x) & 0xffffffff)

typedef enum {
	DT_NULL = 0,
	DT_RELA = 7,
	DT_RELASZ = 8,
	DT_RELAENT = 9
} ElfDynTag;

typedef enum {
	R_X86_64_NONE = 0,
	R_X86_64_RELATIVE = 8
} Elf_x86_64_RelType;
