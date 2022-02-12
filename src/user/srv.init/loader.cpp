#include <stdio.h>
#include <string.h>

#include <bootproto/init.h>
#include <elf/file.h>
#include <elf/headers.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/syscalls.h>
#include <util/enum_bitfields.h>

using bootproto::module_flags;
using bootproto::module_program;

extern j6_handle_t __handle_self;
extern j6_handle_t __handle_sys;

constexpr uintptr_t load_addr = 0xf8000000;
constexpr size_t stack_size = 0x10000;
constexpr uintptr_t stack_top = 0x80000000000;

bool
load_program(const module_program &prog, j6_handle_t sys, char *err_msg)
{
    if (prog.mod_flags && module_flags::no_load) {
        sprintf(err_msg, "  skipping pre-loaded program module '%s' at %lx", prog.filename, prog.base_address);
        return true;
    }

    j6_handle_t elf_vma = j6_handle_invalid;
    j6_status_t res = j6_system_map_phys(__handle_sys, &elf_vma, prog.base_address, prog.size, 0);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': creating physical vma: %lx", prog.filename, res);
        return false;
    }

    res = j6_vma_map(elf_vma, __handle_self, prog.base_address);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': mapping vma: %lx", prog.filename, res);
        return false;
    }

    const void *addr = reinterpret_cast<const void *>(prog.base_address);
    elf::file progelf {addr, prog.size};

    if (!progelf.valid()) {
        sprintf(err_msg, "  ** error loading program '%s': ELF is invalid", prog.filename);
        return false;
    }

    j6_handle_t proc = j6_handle_invalid;
    res = j6_process_create(&proc);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': creating process: %lx", prog.filename, res);
        return false;
    }

    res = j6_process_give_handle(proc, sys, nullptr);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': giving system handle: %lx", prog.filename, res);
        return false;
    }

    for (auto &seg : progelf.programs()) {
        if (seg.type != elf::segment_type::load)
            continue;

        // TODO: way to remap VMA as read-only if there's no write flag on
        // the segment
        unsigned long flags = j6_vm_flag_write;
        if (seg.flags && elf::segment_flags::exec)
            flags |= j6_vm_flag_exec;

        j6_handle_t sub_vma = j6_handle_invalid;
        res = j6_vma_create_map(&sub_vma, seg.mem_size, load_addr, flags);
        if (res != j6_status_ok) {
            sprintf(err_msg, "  ** error loading program '%s': creating sub vma: %lx", prog.filename, res);
            return false;
        }

        void *src = reinterpret_cast<void *>(prog.base_address + seg.offset);
        void *dest = reinterpret_cast<void *>(load_addr);
        memcpy(dest, src, seg.file_size);

        res = j6_vma_map(sub_vma, proc, seg.vaddr);
        if (res != j6_status_ok) {
            sprintf(err_msg, "  ** error loading program '%s': mapping sub vma to child: %lx", prog.filename, res);
            return false;
        }

        res = j6_vma_unmap(sub_vma, __handle_self);
        if (res != j6_status_ok) {
            sprintf(err_msg, "  ** error loading program '%s': unmapping sub vma: %lx", prog.filename, res);
            return false;
        }
    }

    j6_handle_t stack_vma = j6_handle_invalid;
    res = j6_vma_create_map(&stack_vma, stack_size, load_addr, j6_vm_flag_write);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': creating stack vma: %lx", prog.filename, res);
        return false;
    }

    uint64_t *stack = reinterpret_cast<uint64_t*>(load_addr + stack_size);
    memset(stack - 512, 0, 512 * sizeof(uint64_t)); // Zero top page

    res = j6_vma_map(stack_vma, proc, stack_top-stack_size);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': mapping stack vma: %lx", prog.filename, res);
        return false;
    }

    res = j6_vma_unmap(stack_vma, __handle_self);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': unmapping stack vma: %lx", prog.filename, res);
        return false;
    }

    j6_handle_t thread = j6_handle_invalid;
    res = j6_thread_create(&thread, proc, stack_top - 6*sizeof(uint64_t), progelf.entrypoint());
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': creating thread: %lx", prog.filename, res);
        return false;
    }

    res = j6_vma_unmap(elf_vma, __handle_self);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': unmapping elf vma: %lx", prog.filename, res);
        return false;
    }

    return true;
}

