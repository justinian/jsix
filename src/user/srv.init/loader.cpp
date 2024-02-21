#include <stdio.h>
#include <string.h>

#include <bootproto/init.h>
#include <elf/file.h>
#include <elf/headers.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/init.h>
#include <j6/protocols.h>
#include <j6/syscalls.h>
#include <j6/syslog.hh>
#include <util/xoroshiro.h>

#include "j6romfs.h"
#include "loader.h"

using bootproto::module;

static constexpr size_t MiB = 0x10'0000ull;
static constexpr size_t stack_size = 16 * MiB;
static constexpr uintptr_t stack_top = 0x7f0'0000'0000;

static util::xoroshiro256pp rng {0x123456};

inline uintptr_t align_up(uintptr_t a) { return ((a-1) & ~(MiB-1)) + MiB; }

j6_handle_t
map_phys(j6_handle_t sys, uintptr_t phys, size_t len, j6_vm_flags flags)
{
    j6_handle_t vma = j6_handle_invalid;
    j6_status_t res = j6_system_map_phys(sys, &vma, phys, len, flags);
    if (res != j6_status_ok)
        return j6_handle_invalid;

    res = j6_vma_map(vma, 0, &phys, j6_vm_flag_exact);
    if (res != j6_status_ok)
        return j6_handle_invalid;

    return vma;
}

void
stack_push_sentinel(uint8_t *&stack)
{
    static constexpr size_t size = sizeof(j6_arg_header);

    stack -= size;
    memset(stack, 0, size);
    j6_arg_header *header = reinterpret_cast<j6_arg_header*>(stack);
    header->type = j6_arg_type_none;
    header->size = size;
}

template <typename T> T *
stack_push(uint8_t *&stack, size_t extra)
{
    size_t len = sizeof(T) + extra;
    size_t size = (len + 7) & ~7ull;
    stack -= size;
    memset(stack, 0, sizeof(T));
    T * arg = reinterpret_cast<T*>(stack);
    arg->header.type = T::type_id;
    arg->header.size = len;
    return arg;
}

uintptr_t
load_program_into(j6_handle_t proc, elf::file &file, uintptr_t image_base, const char *path)
{
    uintptr_t eop = 0; // end of program

    for (auto &seg : file.segments()) {
        if (seg.type != elf::segment_type::load)
            continue;

        uintptr_t addr = 0;

        // TODO: way to remap VMA as read-only if there's no write flag on
        // the segment
        unsigned long flags = j6_vm_flag_write;
        if (seg.flags && elf::segment_flags::exec)
            flags |= j6_vm_flag_exec;

        uintptr_t start = file.base() + seg.offset;
        size_t prologue = seg.vaddr & 0xfff;
        size_t epilogue = seg.mem_size - seg.file_size;

        j6_handle_t sub_vma = j6_handle_invalid;
        j6_status_t res = j6_vma_create_map(&sub_vma, seg.mem_size+prologue, &addr, flags);
        if (res != j6_status_ok) {
            j6::syslog("  ** error loading ELF '%s': creating sub vma: %lx", path, res);
            return 0;
        }

        uint8_t *src = reinterpret_cast<uint8_t *>(start);
        uint8_t *dest = reinterpret_cast<uint8_t *>(addr);
        memset(dest, 0, prologue);
        memcpy(dest+prologue, src, seg.file_size);
        memset(dest+prologue+seg.file_size, 0, epilogue);

        // end of segment
        uintptr_t eos = image_base + seg.vaddr + seg.mem_size + prologue;
        if (eos > eop)
            eop = eos;

        uintptr_t start_addr = (image_base + seg.vaddr) & ~0xfffull;
        j6::syslog("Mapping segment from %s at %012lx - %012lx", path, start_addr, start_addr+seg.mem_size);
        res = j6_vma_map(sub_vma, proc, &start_addr, j6_vm_flag_exact);
        if (res != j6_status_ok) {
            j6::syslog("  ** error loading ELF '%s': mapping sub vma to child: %lx", path, res);
            return 0;
        }

        res = j6_vma_unmap(sub_vma, 0);
        if (res != j6_status_ok) {
            j6::syslog("  ** error loading ELF '%s': unmapping sub vma: %lx", path, res);
            return 0;
        }
    }

    return eop;
}

static bool
give_handle(j6_handle_t proc, j6_handle_t h, const char *name)
{
    if (h != j6_handle_invalid) {
        j6_status_t res = j6_process_give_handle(proc, h);
        if (res != j6_status_ok) {
            j6::syslog("  ** error loading program: giving %s handle: %lx", name, res);
            return false;
        }
    }

    return true;
}

static j6_handle_t
create_process(j6_handle_t sys, j6_handle_t slp, j6_handle_t vfs)
{
    j6_handle_t proc = j6_handle_invalid;
    j6_status_t res = j6_process_create(&proc);
    if (res != j6_status_ok) {
        j6::syslog("  ** error loading program: creating process: %lx", res);
        return j6_handle_invalid;
    }

    if (!give_handle(proc, sys, "system")) return j6_handle_invalid;
    if (!give_handle(proc, slp, "SLP")) return j6_handle_invalid;
    if (!give_handle(proc, vfs, "VFS")) return j6_handle_invalid;
    return proc;
}

static util::buffer
load_file(const j6romfs::fs &fs, const char *path)
{
    const j6romfs::inode *in = fs.lookup_inode(path);
    if (!in || in->type != j6romfs::inode_type::file)
        return {};

    j6::syslog("  Loading file: %s", path);

    uint8_t *data = new uint8_t [in->size];
    util::buffer program {data, in->size};

    fs.load_inode_data(in, program);
    return program;
}


bool
load_program(
        const char *path, const j6romfs::fs &fs,
        j6_handle_t sys, j6_handle_t slp, j6_handle_t vfs,
        const module *arg)
{
    j6::syslog("Loading program '%s' into new process", path);
    util::buffer program_data = load_file(fs, path);
    if (!program_data.pointer)
        return false;

    elf::file program_elf {program_data};

    bool dyn = program_elf.type() == elf::filetype::shared;
    uintptr_t program_image_base = 0;
    if (dyn)
        program_image_base = (rng.next() & 0xffe0 + 16) << 20;

    if (!program_elf.valid(dyn ? elf::filetype::shared : elf::filetype::executable)) {
        j6::syslog("  ** error loading program '%s': ELF is invalid", path);
        return false;
    }

    j6_handle_t proc = create_process(sys, slp, vfs);
    uintptr_t eop = load_program_into(proc, program_elf, program_image_base, path);
    if (!eop)
        return false;

    uintptr_t stack_addr = 0;
    j6_handle_t stack_vma = j6_handle_invalid;
    j6_status_t res = j6_vma_create_map(&stack_vma, stack_size, &stack_addr, j6_vm_flag_write);
    if (res != j6_status_ok) {
        j6::syslog("  ** error loading program '%s': creating stack vma: %lx", path, res);
        return false;
    }

    uint8_t *stack_orig = reinterpret_cast<uint8_t*>(stack_addr + stack_size);
    uint8_t *stack = stack_orig;
    memset(stack - 4096, 0, 4096); // Zero top page
    stack_push_sentinel(stack);

    if (arg) {
        size_t data_size = arg->bytes - sizeof(module);
        j6_arg_driver *driver_arg = stack_push<j6_arg_driver>(stack, data_size);
        driver_arg->device = arg->type_id;

        const uint8_t *arg_data = arg->data<uint8_t>();
        memcpy(driver_arg->data, arg_data, data_size);
    }

    uintptr_t entrypoint = program_elf.entrypoint() + program_image_base;

    if (dyn) {
        stack_push_sentinel(stack);
        j6_arg_loader *loader_arg = stack_push<j6_arg_loader>(stack, 0);
        const elf::file_header *h = program_elf.header();
        loader_arg->image_base = program_image_base;

        const elf::section_header *got_section = program_elf.get_section_by_name(".got.plt");
        if (got_section)
            loader_arg->got = reinterpret_cast<uintptr_t*>(program_image_base + got_section->addr);

        // The dynamic linker will offset the entrypoint, don't do it here.
        loader_arg->entrypoint = program_elf.entrypoint();

        j6_arg_handles *handles_arg = stack_push<j6_arg_handles>(stack, 2 * sizeof(j6_arg_handle_entry));
        handles_arg->nhandles = 2;
        handles_arg->handles[0].handle = slp;
        handles_arg->handles[0].proto = j6::proto::sl::id;
        handles_arg->handles[1].handle = vfs;
        handles_arg->handles[1].proto = j6::proto::vfs::id;

        // Align the stack to be one word short of 16-byte aligned, so
        // that the arg address will be aligned when pushed
        while ((reinterpret_cast<uintptr_t>(stack) & 0xf) != 0x8) --stack;

        // Push the args list address itself
        stack -= sizeof(uintptr_t);
        uintptr_t *args_addr = reinterpret_cast<uintptr_t*>(stack);
        *args_addr = stack_top - (stack_orig - reinterpret_cast<uint8_t*>(handles_arg));

        uintptr_t ldso_image_base = (eop & ~(MiB-1)) + MiB;

        for (auto seg : program_elf.segments()) {
            if (seg.type == elf::segment_type::interpreter) {
                const char *ldso_path = reinterpret_cast<const char*>(program_elf.base() + seg.offset);
                util::buffer ldso_data = load_file(fs, ldso_path);
                if (!ldso_data.pointer)
                    return false;

                elf::file ldso_elf {ldso_data};
                if (!ldso_elf.valid(elf::filetype::shared)) {
                    j6::syslog("  ** error loading dynamic linker for program '%s': ELF is invalid", path);
                    return false;
                }

                uintptr_t eop = load_program_into(proc, ldso_elf, ldso_image_base, ldso_path);
                eop = (eop & ~0xfffffull) + 0x100000;
                loader_arg->loader_base = ldso_image_base;
                loader_arg->start_addr = eop;
                entrypoint = ldso_elf.entrypoint() + ldso_image_base;
                delete [] reinterpret_cast<uint8_t*>(ldso_data.pointer);
                break;
            }
        }
    }

    uintptr_t stack_base = stack_top-stack_size;
    res = j6_vma_map(stack_vma, proc, &stack_base, j6_vm_flag_exact);
    if (res != j6_status_ok) {
        j6::syslog("  ** error loading program '%s': mapping stack vma: %lx", path, res);
        return false;
    }

    res = j6_vma_unmap(stack_vma, 0);
    if (res != j6_status_ok) {
        j6::syslog("  ** error loading program '%s': unmapping stack vma: %lx", path, res);
        return false;
    }

    j6_handle_t thread = j6_handle_invalid;
    uintptr_t target_stack = stack_top - (stack_orig - stack);
    target_stack &= ~0xfull; // Align to 16-byte stack
    res = j6_thread_create(&thread, proc, target_stack, entrypoint, program_image_base, 0);
    if (res != j6_status_ok) {
        j6::syslog("  ** error loading program '%s': creating thread: %lx", path, res);
        return false;
    }

    // TODO: smart pointer this, it's a memory leak if we return early
    delete [] reinterpret_cast<uint8_t*>(program_data.pointer);
    return true;
}

