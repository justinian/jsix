#pragma once
#include <stdint.h>
#include <util/enum_bitfields.h>

namespace elf {

enum class wordsize : uint8_t { invalid, bits32, bits64 };
enum class encoding : uint8_t { invalid, lsb, msb };
enum class osabi : uint8_t { sysV, hpux, standalone = 255 };
enum class machine : uint16_t { none, x64 = 0x3e };

enum class filetype : uint16_t
{
    none,
    relocatable,
    executable,
    shared,
    core
};


struct file_header
{
    uint32_t magic;

    wordsize word_size;
    encoding endianness;
    uint8_t ident_version;
    osabi os_abi;

    uint64_t reserved;

    filetype file_type;
    machine machine_type;

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


enum class segment_type : uint32_t { null, load, dynamic, interpreter, note };
enum class segment_flags : uint32_t
{
    none  = 0x00,
    exec  = 0x01,
    write = 0x02,
    read  = 0x04,
};
is_bitfield(segment_flags);

struct segment_header
{
    segment_type type;
    segment_flags flags;
    uint64_t offset;

    uint64_t vaddr;
    uint64_t paddr;

    uint64_t file_size;
    uint64_t mem_size;

    uint64_t align;
} __attribute__ ((packed));


enum class section_type : uint32_t { null, progbits };
enum class section_flags : uint64_t
{
    write = 0x01,
    alloc = 0x02,
    exec  = 0x04,
};

struct section_header
{
    uint32_t name_offset;
    section_type type;
    section_flags flags;
    uint64_t addr;
    uint64_t offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t align;
    uint64_t entry_size;
} __attribute__ ((packed));

} // namespace elf

is_bitfield(elf::section_flags);
