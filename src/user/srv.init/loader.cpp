#include <stdio.h>
#include <string.h>

#include <bootproto/init.h>
#include <elf/file.h>
#include <elf/headers.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/syscalls.h>
#include <util/enum_bitfields.h>

#include "loader.h"

using bootproto::module;

extern j6_handle_t __handle_self;

constexpr uintptr_t load_addr = 0xf8000000;
constexpr size_t stack_size = 0x10000;
constexpr uintptr_t stack_top = 0x80000000000;

j6_handle_t
map_phys(j6_handle_t sys, uintptr_t phys, size_t len, uintptr_t addr)
{
    j6_handle_t vma = j6_handle_invalid;
    j6_status_t res = j6_system_map_phys(sys, &vma, phys, len, 0);
    if (res != j6_status_ok)
        return j6_handle_invalid;

    if (!addr)
        addr = phys;

    res = j6_vma_map(vma, __handle_self, addr);
    if (res != j6_status_ok)
        return j6_handle_invalid;

    return vma;
}

bool
load_program(
        const char *name,
        util::const_buffer data,
        j6_handle_t sys, j6_handle_t slp,
        char *err_msg)
{
    uintptr_t base_address = reinterpret_cast<uintptr_t>(data.pointer);

    elf::file progelf {data};

    if (!progelf.valid()) {
        sprintf(err_msg, "  ** error loading program '%s': ELF is invalid", name);
        return false;
    }

    j6_handle_t proc = j6_handle_invalid;
    j6_status_t res = j6_process_create(&proc);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': creating process: %lx", name, res);
        return false;
    }

    res = j6_process_give_handle(proc, sys);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': giving system handle: %lx", name, res);
        return false;
    }

    res = j6_process_give_handle(proc, slp);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': giving SLP handle: %lx", name, res);
        return false;
    }

    for (auto &seg : progelf.segments()) {
        if (seg.type != elf::segment_type::load)
            continue;

        // TODO: way to remap VMA as read-only if there's no write flag on
        // the segment
        unsigned long flags = j6_vm_flag_write;
        if (seg.flags && elf::segment_flags::exec)
            flags |= j6_vm_flag_exec;

        uintptr_t start = base_address + seg.offset;
        size_t prologue = seg.vaddr & 0xfff;
        size_t epilogue = seg.mem_size - (prologue+seg.file_size);

        j6_handle_t sub_vma = j6_handle_invalid;
        res = j6_vma_create_map(&sub_vma, seg.mem_size+prologue, load_addr, flags);
        if (res != j6_status_ok) {
            sprintf(err_msg, "  ** error loading program '%s': creating sub vma: %lx", name, res);
            return false;
        }

        uint8_t *src = reinterpret_cast<uint8_t *>(start);
        uint8_t *dest = reinterpret_cast<uint8_t *>(load_addr);
        memset(dest, 0, prologue);
        memcpy(dest+prologue, src, seg.file_size);
        memset(dest+prologue+seg.file_size, 0, epilogue);

        res = j6_vma_map(sub_vma, proc, seg.vaddr & ~0xfffull);
        if (res != j6_status_ok) {
            sprintf(err_msg, "  ** error loading program '%s': mapping sub vma to child: %lx", name, res);
            return false;
        }

        res = j6_vma_unmap(sub_vma, __handle_self);
        if (res != j6_status_ok) {
            sprintf(err_msg, "  ** error loading program '%s': unmapping sub vma: %lx", name, res);
            return false;
        }
    }

    j6_handle_t stack_vma = j6_handle_invalid;
    res = j6_vma_create_map(&stack_vma, stack_size, load_addr, j6_vm_flag_write);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': creating stack vma: %lx", name, res);
        return false;
    }

    uint64_t *stack = reinterpret_cast<uint64_t*>(load_addr + stack_size);
    memset(stack - 512, 0, 512 * sizeof(uint64_t)); // Zero top page

    res = j6_vma_map(stack_vma, proc, stack_top-stack_size);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': mapping stack vma: %lx", name, res);
        return false;
    }

    res = j6_vma_unmap(stack_vma, __handle_self);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': unmapping stack vma: %lx", name, res);
        return false;
    }

    j6_handle_t thread = j6_handle_invalid;
    res = j6_thread_create(&thread, proc, stack_top - 6*sizeof(uint64_t), progelf.entrypoint(), 0, 0);
    if (res != j6_status_ok) {
        sprintf(err_msg, "  ** error loading program '%s': creating thread: %lx", name, res);
        return false;
    }

    return true;
}

