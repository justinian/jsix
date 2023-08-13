#include <stdlib.h>
#include <j6/syslog.hh>
#include <util/counted.h>
#include "relocate.h"

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

struct dyn_values {
    uintptr_t pltrelsz;
    uintptr_t pltgot;
    uintptr_t hash;
    uintptr_t pltrel;
    uintptr_t strtab;
    uintptr_t symtab;
    uintptr_t rela;
    uintptr_t relasz;
    uintptr_t relaent;
    uintptr_t strsz;
    uintptr_t syment;
    uintptr_t jmprel;
    uintptr_t relacount;
};

#define read_dyn_value(name)        \
    case dyn_type::name:            \
        values.name = dyn.value;    \
        break;

void
read_dynamic_values(uintptr_t base, uintptr_t dyn_section, dyn_values &values)
{
    const dyn_entry *dyns = reinterpret_cast<const dyn_entry*>(dyn_section + base);

    unsigned i = 0;
    while (true) {
        const dyn_entry &dyn = dyns[i++];
        switch (dyn.tag) {
        case dyn_type::null:
            return;

        read_dyn_value(pltrelsz);
        read_dyn_value(pltgot);
        read_dyn_value(pltrel);
        read_dyn_value(strtab);
        read_dyn_value(symtab);
        read_dyn_value(rela);
        read_dyn_value(relasz);
        read_dyn_value(relaent);
        read_dyn_value(strsz);
        read_dyn_value(syment);
        read_dyn_value(jmprel);   
        read_dyn_value(relacount);   

        case dyn_type::gnu_hash:
            values.hash = dyn.value;
            break;

        default:
            break;
        }
    }
}

#undef read_dyn_value(name)

class string_table :
    private util::counted<const char>
{
public:
    string_table(const char *start, size_t size) :
        util::counted<const char> {start, size} {}

    const char *lookup(size_t offset) {
        if (offset > count) return nullptr;
        return pointer + offset;
    }
};

struct symbol
{
    uint32_t name;
    uint8_t type : 4;
    uint8_t binding : 4;
    uint8_t _reserved0;
    uint16_t section;
    uintptr_t address;
    size_t size;
};

struct gnu_hash_table
{
    uint32_t bucket_count;
    uint32_t start_symbol;
    uint32_t bloom_count;
    uint32_t bloom_shift;
    uint64_t bloom [0];
};

enum class reloc : uint32_t {
    relative = 8,
};

struct rela
{
    uintptr_t address;
    reloc type;
    uint32_t symbol;
    ptrdiff_t offset;
};

template <typename T> T off(uintptr_t p, uintptr_t base) { return reinterpret_cast<T>(p ? p + base : 0); }

void
relocate_image(uintptr_t base, uintptr_t dyn_section)
{
    dyn_values values = {0};
    read_dynamic_values(base, dyn_section, values);

    string_table strtab {
        reinterpret_cast<const char *>(values.strtab + base),
        values.strsz,
    };

    symbol *symtab = off<symbol *>(values.symtab, base);
    gnu_hash_table *hashtab = off<gnu_hash_table *>(values.hash, base);

    uintptr_t *jmprel = off<uintptr_t *>(values.jmprel, base);
    uintptr_t *plt = off<uintptr_t *>(values.pltgot, base);

    size_t nrela = values.relacount;
    for (size_t i = 0; i < values.relacount; ++i) {
        rela *rel = off<rela*>(values.rela + i * values.relaent, base);
        switch (rel->type)
        {
        case reloc::relative:
            *reinterpret_cast<uint64_t*>(rel->address + base) = base + rel->offset;
            break;
        
        default:
            j6::syslog("Unknown relocation type %d", rel->type);
            exit(126);
            break;
        }
    }
}