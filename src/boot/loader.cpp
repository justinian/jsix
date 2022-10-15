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
    const descriptor &desc)
{
    status_line status(L"Loading file", desc.path);

    fs::file file = disk.open(desc.path);
    util::buffer b = file.load();

    //console::print(L"    Loaded at: 0x%lx, %d bytes\r\n", b.data, b.size);
    return b;
}


static void
create_module(util::buffer data, const descriptor &desc, bool loaded)
{
    size_t path_len = wstrlen(desc.path);
    bootproto::module_program *mod = g_alloc.allocate_module<bootproto::module_program>(path_len);
    mod->mod_type = bootproto::module_type::program;
    mod->base_address = reinterpret_cast<uintptr_t>(data.pointer);
    mod->size = data.count;
    if (loaded)
        mod->mod_flags = static_cast<bootproto::module_flags>(
            static_cast<uint8_t>(mod->mod_flags) |
            static_cast<uint8_t>(bootproto::module_flags::no_load));

    // TODO: support non-ascii path characters and do real utf-16 to utf-8
    // conversion
    for (int i = 0; i < path_len; ++i) {
        char c = desc.path[i];
        mod->filename[i] = c == '\\' ? '/' : c;
    }
    mod->filename[path_len] = 0;
}

bootproto::program *
load_program(
    fs::file &disk,
    const descriptor &desc,
    bool add_module)
{
    status_line status(L"Loading program", desc.desc);

    util::buffer data = load_file(disk, desc);

    if (add_module)
        create_module(data, desc, true);

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
        void *source = util::offset_pointer(data.pointer, seg.offset);
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
    const descriptor &desc)
{
    status_line status(L"Loading module", desc.desc);

    util::buffer data = load_file(disk, desc);
    create_module(data, desc, false);
}

void
verify_kernel_header(bootproto::program &program)
{
    status_line status(L"Verifying kernel header");

    const bootproto::header *header =
        reinterpret_cast<const bootproto::header *>(program.sections[0].phys_addr);

    if (header->magic != bootproto::header_magic)
        error::raise(uefi::status::load_error, L"Bad kernel magic number");

    if (header->length < sizeof(bootproto::header))
        error::raise(uefi::status::load_error, L"Bad kernel header length");

    if (header->version < bootproto::min_header_version)
        error::raise(uefi::status::unsupported, L"Kernel header version not supported");

    console::print(L"    Loaded kernel vserion: %d.%d.%d %x\r\n",
            header->version_major, header->version_minor, header->version_patch,
            header->version_gitsha);

    /*
    for (auto &section : program.sections)
        console::print(L"    Section: p:0x%lx v:0x%lx fs:0x%x ms:0x%x\r\n",
            section.phys_addr, section.virt_addr, section.file_size, section.mem_size);
    */
}

} // namespace loader
} // namespace boot
