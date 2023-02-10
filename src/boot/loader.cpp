#include <uefi/boot_services.h>
#include <uefi/types.h>

#include <elf/file.h>
#include <elf/headers.h>
#include <util/pointers.h>

#include "allocator.h"
#include "bootconfig.h"
#include "console.h"
#include "error.h"
#include "fs.h"
#include "loader.h"
#include "memory.h"
#include "paging.h"
#include "status.h"

namespace boot {
namespace loader {

using memory::alloc_type;

util::buffer
load_file(fs::file &disk, const wchar_t *path)
{
    status_line status(L"Loading file", path);

    fs::file file = disk.open(path);
    util::buffer b = file.load();

    //console::print(L"    Loaded at: 0x%lx, %d bytes\r\n", b.data, b.size);
    return b;
}

void
verify_kernel_header(elf::file &kernel, util::const_buffer data)
{
    status_line status {L"Verifying kernel header"};

    // The header should be the first non-null section
    const elf::section_header &sect = kernel.sections()[1];

    const bootproto::header *header =
        reinterpret_cast<const bootproto::header *>(
            util::offset_pointer(data.pointer, sect.offset));

    if (header->magic != bootproto::header_magic)
        error::raise(uefi::status::load_error, L"Bad kernel magic number");

    if (header->length < sizeof(bootproto::header))
        error::raise(uefi::status::load_error, L"Bad kernel header length");

    if (header->version < bootproto::min_header_version)
        error::raise(uefi::status::unsupported, L"Kernel header version not supported");

    console::print(L"    Loaded kernel vserion: %d.%d.%d %x\r\n",
            header->version_major, header->version_minor, header->version_patch,
            header->version_gitsha);
}

inline void
elf_error(const elf::file &elf, util::const_buffer data)
{
    auto *header = elf.header();
    console::print(L"        progam size: %d\r\n", data.count);
    console::print(L"          word size: %d\r\n", header->word_size);
    console::print(L"         endianness: %d\r\n", header->endianness);
    console::print(L"  ELF ident version: %d\r\n", header->ident_version);
    console::print(L"             OS ABI: %d\r\n", header->os_abi);
    console::print(L"          file type: %d\r\n", header->file_type);
    console::print(L"       machine type: %d\r\n", header->machine_type);
    console::print(L"        ELF version: %d\r\n", header->version);

    error::raise(uefi::status::load_error, L"ELF file not valid");
}

inline uintptr_t
allocate_bss(elf::segment_header seg)
{
    size_t page_count = memory::bytes_to_pages(seg.mem_size);
    void *pages = g_alloc.allocate_pages(page_count, alloc_type::program, true);
    return reinterpret_cast<uintptr_t>(pages);
}


void
parse_program(const wchar_t *name, util::const_buffer data, bootproto::program &program)
{
    status_line status(L"Preparing program", name);

    elf::file elf {data};
    if (!elf.valid())
        elf_error(elf, data); // does not return

    size_t num_sections = 0;
    for (auto &seg : elf.segments()) {
        if (seg.type == elf::segment_type::load)
            ++num_sections;
    }

    bootproto::program_section *sections =
        new bootproto::program_section [num_sections];

    size_t next_section = 0;
    for (auto &seg : elf.segments()) {
        if (seg.type != elf::segment_type::load)
            continue;

        bootproto::program_section &section = sections[next_section++];
        section.phys_addr = elf.base() + seg.offset;
        section.virt_addr = seg.vaddr;
        section.size = seg.mem_size;
        section.type = static_cast<bootproto::section_flags>(seg.flags);

        if (seg.mem_size != seg.file_size)
            section.phys_addr = allocate_bss(seg);
    }

    program.sections = { .pointer = sections, .count = num_sections };
    program.phys_base = elf.base();
    program.entrypoint = elf.entrypoint();
}

uintptr_t
load_program(
    util::const_buffer data,
    const wchar_t *name,
    paging::pager &pager,
    bool verify)
{
    using util::bits::has;

    status_line status(L"Loading program", name);

    elf::file elf {data};
    if (!elf.valid())
        elf_error(elf, data); // does not return

    if (verify)
        verify_kernel_header(elf, data);

    size_t num_sections = 0;
    for (auto &seg : elf.segments()) {
        if (seg.type == elf::segment_type::load)
            ++num_sections;
    }

    for (auto &seg : elf.segments()) {
        if (seg.type != elf::segment_type::load)
            continue;

        uintptr_t phys_addr = elf.base() + seg.offset;
        if (seg.mem_size != seg.file_size)
            phys_addr = allocate_bss(seg);

        pager.map_pages(phys_addr, seg.vaddr,
            memory::bytes_to_pages(seg.mem_size),
            has(seg.flags, elf::segment_flags::write),
            has(seg.flags, elf::segment_flags::exec));
    }

    return elf.entrypoint();
}

void
load_module(
    fs::file &disk,
    const wchar_t *name,
    const wchar_t *path,
    bootproto::module_type type)
{
    status_line status(L"Loading module", name);

    bootproto::module *mod = g_alloc.allocate_module(sizeof(util::buffer));
    mod->type = type;

    util::buffer *data = mod->data<util::buffer>();
    *data = load_file(disk, path);
}

} // namespace loader
} // namespace boot
