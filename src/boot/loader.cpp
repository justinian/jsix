#include <uefi/boot_services.h>
#include <uefi/types.h>

#include <bootproto/init.h>
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
load_file(
    fs::file &disk,
    const wchar_t *path)
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

bootproto::program *
load_program(
    fs::file &disk,
    const wchar_t *name,
    const descriptor &desc,
    bool verify)
{
    status_line status(L"Loading program", name);

    util::const_buffer data = load_file(disk, desc.path);

    elf::file program(data.pointer, data.count);
    if (!program.valid()) {
        auto *header = program.header();
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

    if (verify)
        verify_kernel_header(program, data);

    size_t num_sections = 0;
    for (auto &seg : program.programs()) {
        if (seg.type == elf::segment_type::load)
            ++num_sections;
    }

    bootproto::program_section *sections = new bootproto::program_section [num_sections];

    size_t next_section = 0;
    for (auto &seg : program.programs()) {
        if (seg.type != elf::segment_type::load)
            continue;

        bootproto::program_section &section = sections[next_section++];

        uintptr_t virt_addr = seg.vaddr;
        size_t mem_size = seg.mem_size;

        // Page-align the section, which may require increasing the size
        size_t prelude = virt_addr & 0xfff;
        mem_size += prelude;
        virt_addr &= ~0xfffull;

        size_t page_count = memory::bytes_to_pages(mem_size);
        void *pages = g_alloc.allocate_pages(page_count, alloc_type::program, true);
        const void *source = util::offset_pointer(data.pointer, seg.offset);
        g_alloc.copy(util::offset_pointer(pages, prelude), source, seg.file_size);
        section.phys_addr = reinterpret_cast<uintptr_t>(pages);
        section.virt_addr = virt_addr;
        section.size = mem_size;
        section.type = static_cast<bootproto::section_flags>(seg.flags);
    }

    bootproto::program *prog = new bootproto::program;
    prog->sections = { .pointer = sections, .count = num_sections };
    prog->phys_base = program.base();
    prog->entrypoint = program.entrypoint();
    return prog;
}

void
load_module(
    fs::file &disk,
    const wchar_t *name,
    const wchar_t *path,
    bootproto::module_type type,
    uint16_t subtype)
{
    status_line status(L"Loading module", name);

    bootproto::module *mod = g_alloc.allocate_module();
    mod->type = type;
    mod->subtype = subtype;
    mod->data = load_file(disk, path);
}

} // namespace loader
} // namespace boot
