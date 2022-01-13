#include <utility>

#include <arch/memory.h>
#include <bootproto/kernel.h>
#include <util/no_construct.h>

#include "assert.h"
#include "device_manager.h"
#include "frame_allocator.h"
#include "gdt.h"
#include "heap_allocator.h"
#include "io.h"
#include "log.h"
#include "memory.h"
#include "msr.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "objects/system.h"
#include "objects/vm_area.h"
#include "vm_space.h"

using bootproto::allocation_register;
using bootproto::section_flags;

extern "C" void initialize_main_thread();
extern "C" uintptr_t initialize_main_user_stack();

// These objects are initialized _before_ global constructors are called,
// so we don't want them to have global constructors at all, lest they
// overwrite the previous initialization.
static util::no_construct<heap_allocator> __g_kernel_heap_storage;
heap_allocator &g_kernel_heap = __g_kernel_heap_storage.value;

static util::no_construct<frame_allocator> __g_frame_allocator_storage;
frame_allocator &g_frame_allocator = __g_frame_allocator_storage.value;

static util::no_construct<vm_area_untracked> __g_kernel_heap_area_storage;
vm_area_untracked &g_kernel_heap_area = __g_kernel_heap_area_storage.value;

static util::no_construct<vm_area_guarded> __g_kernel_stacks_storage;
vm_area_guarded &g_kernel_stacks = __g_kernel_stacks_storage.value;

vm_area_guarded g_kernel_buffers {
    mem::buffers_offset,
    mem::kernel_buffer_pages,
    mem::buffers_size,
    vm_flags::write};

void * operator new(size_t size)           { return g_kernel_heap.allocate(size); }
void * operator new [] (size_t size)       { return g_kernel_heap.allocate(size); }
void operator delete (void *p) noexcept    { return g_kernel_heap.free(p); }
void operator delete [] (void *p) noexcept { return g_kernel_heap.free(p); }

void * kalloc(size_t size) { return g_kernel_heap.allocate(size); }
void kfree(void *p) { return g_kernel_heap.free(p); }

template <typename T>
uintptr_t
get_physical_page(T *p) {
    return mem::page_align_down(reinterpret_cast<uintptr_t>(p));
}

void
memory_initialize_pre_ctors(bootproto::args &kargs)
{
    using bootproto::frame_block;

    page_table *kpml4 = static_cast<page_table*>(kargs.pml4);

    new (&g_kernel_heap) heap_allocator {mem::heap_offset, mem::heap_size};

    frame_block *blocks = reinterpret_cast<frame_block*>(mem::bitmap_offset);
    new (&g_frame_allocator) frame_allocator {blocks, kargs.frame_blocks.count};

    // Mark all the things the bootloader allocated for us as used
    allocation_register *reg = kargs.allocations;
    while (reg) {
        for (auto &alloc : reg->entries)
            if (alloc.type != bootproto::allocation_type::none)
                g_frame_allocator.used(alloc.address, alloc.count);
        reg = reg->next;
    }

    process *kp = process::create_kernel_process(kpml4);
    vm_space &vm = kp->space();

    vm_area *heap = new (&g_kernel_heap_area)
        vm_area_untracked(mem::heap_size, vm_flags::write);

    vm.add(mem::heap_offset, heap);

    vm_area *stacks = new (&g_kernel_stacks) vm_area_guarded {
        mem::stacks_offset,
        mem::kernel_stack_pages,
        mem::stacks_size,
        vm_flags::write};
    vm.add(mem::stacks_offset, &g_kernel_stacks);

    // Clean out any remaning bootloader page table entries
    for (unsigned i = 0; i < arch::kernel_root_index; ++i)
        kpml4->entries[i] = 0;
}

void
memory_initialize_post_ctors(bootproto::args &kargs)
{
    vm_space &vm = vm_space::kernel_space();
    vm.add(mem::buffers_offset, &g_kernel_buffers);

    g_frame_allocator.free(
        get_physical_page(kargs.page_tables.pointer),
        kargs.page_tables.count);
}

static void
log_mtrrs()
{
    uint64_t mtrrcap = rdmsr(msr::ia32_mtrrcap);
    uint64_t mtrrdeftype = rdmsr(msr::ia32_mtrrdeftype);
    unsigned vcap = mtrrcap & 0xff;
    log::debug(logs::boot, "MTRRs: vcap=%d %s %s def=%02x %s %s",
        vcap,
        (mtrrcap & (1<< 8)) ? "fix" : "",
        (mtrrcap & (1<<10)) ? "wc" : "",
        mtrrdeftype & 0xff,
        (mtrrdeftype & (1<<10)) ? "fe" : "",
        (mtrrdeftype & (1<<11)) ? "enabled" : ""
        );

    for (unsigned i = 0; i < vcap; ++i) {
        uint64_t base = rdmsr(find_mtrr(msr::ia32_mtrrphysbase, i));
        uint64_t mask = rdmsr(find_mtrr(msr::ia32_mtrrphysmask, i));
        log::debug(logs::boot, "       vcap[%2d] base:%016llx mask:%016llx type:%02x %s", i,
            (base & ~0xfffull),
            (mask & ~0xfffull),
            (base & 0xff),
            (mask & (1<<11)) ? "valid" : "");
    }

    msr mtrr_fixed[] = {
        msr::ia32_mtrrfix64k_00000,
        msr::ia32_mtrrfix16k_80000,
        msr::ia32_mtrrfix16k_a0000,
        msr::ia32_mtrrfix4k_c0000,
        msr::ia32_mtrrfix4k_c8000,
        msr::ia32_mtrrfix4k_d0000,
        msr::ia32_mtrrfix4k_d8000,
        msr::ia32_mtrrfix4k_e0000,
        msr::ia32_mtrrfix4k_e8000,
        msr::ia32_mtrrfix4k_f0000,
        msr::ia32_mtrrfix4k_f8000,
    };

    for (int i = 0; i < 11; ++i) {
        uint64_t v = rdmsr(mtrr_fixed[i]);
        log::debug(logs::boot, "      fixed[%2d] %02x %02x %02x %02x %02x %02x %02x %02x", i,
            ((v <<  0) & 0xff), ((v <<  8) & 0xff), ((v << 16) & 0xff), ((v << 24) & 0xff),
            ((v << 32) & 0xff), ((v << 40) & 0xff), ((v << 48) & 0xff), ((v << 56) & 0xff));
    }

    uint64_t pat = rdmsr(msr::ia32_pat);
    static const char *pat_names[] = {"UC ","WC ","XX ","XX ","WT ","WP ","WB ","UC-"};
    log::debug(logs::boot, "      PAT: 0:%s 1:%s 2:%s 3:%s 4:%s 5:%s 6:%s 7:%s",
        pat_names[(pat >> (0*8)) & 7], pat_names[(pat >> (1*8)) & 7],
        pat_names[(pat >> (2*8)) & 7], pat_names[(pat >> (3*8)) & 7],
        pat_names[(pat >> (4*8)) & 7], pat_names[(pat >> (5*8)) & 7],
        pat_names[(pat >> (6*8)) & 7], pat_names[(pat >> (7*8)) & 7]);
}


void
load_init_server(bootproto::program &program, uintptr_t modules_address)
{
    process *p = new process;
    p->add_handle(&system::get());

    vm_space &space = p->space();
    for (const auto &sect : program.sections) {
        vm_flags flags =
            ((sect.type && section_flags::execute) ? vm_flags::exec : vm_flags::none) |
            ((sect.type && section_flags::write) ? vm_flags::write : vm_flags::none);

        vm_area *vma = new vm_area_fixed(sect.phys_addr, sect.size, flags);
        space.add(sect.virt_addr, vma);
    }

    uint64_t iopl = (3ull << 12);

    thread *main = p->create_thread();
    main->add_thunk_user(program.entrypoint, 0, iopl);
    main->set_state(thread::state::ready);

    // Hacky: No process exists to have created a stack for init; it needs to create
    // its own stack. We take advantage of that to use rsp to pass it the init modules
    // address.
    auto *tcb = main->tcb();
    tcb->rsp3 = modules_address;
}
