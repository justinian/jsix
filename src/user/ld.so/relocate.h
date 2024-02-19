#pragma once
/// \file relocate.h
/// Image relocation services

#include <stddef.h>
#include <stdint.h>

enum class dyn_type : uint64_t {
    null, needed, pltrelsz, pltgot, hash, strtab, symtab, rela, relasz, relaent,
    strsz, syment, init, fini, soname, rpath, symbolic, rel, relsz, relent, pltrel,
    debug, textrel, jmprel, bind_now, init_array, fini_array, init_arraysz, fini_arraysz,
    gnu_hash = 0x6ffffef5, relacount = 0x6ffffff9,
};

struct dyn_entry {
    dyn_type tag;
    uintptr_t value;
};

enum class reloc : uint32_t {
    glob_dat = 6,
    jump_slot = 7,
    relative = 8,
};

struct rela
{
    uintptr_t address;
    reloc type;
    uint32_t symbol;
    ptrdiff_t offset;
};
