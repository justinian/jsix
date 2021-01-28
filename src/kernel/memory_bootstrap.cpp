#include <utility>

#include "kernel_args.h"
#include "j6/init.h"

#include "kutil/assert.h"
#include "kutil/heap_allocator.h"
#include "kutil/no_construct.h"

#include "device_manager.h"
#include "frame_allocator.h"
#include "io.h"
#include "log.h"
#include "msr.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "objects/system.h"
#include "objects/vm_area.h"
#include "vm_space.h"

using memory::heap_start;
using memory::kernel_max_heap;

using namespace kernel;

extern "C" void initialize_main_thread();
extern "C" uintptr_t initialize_main_user_stack();

// These objects are initialized _before_ global constructors are called,
// so we don't want them to have global constructors at all, lest they
// overwrite the previous initialization.
static kutil::no_construct<kutil::heap_allocator> __g_kernel_heap_storage;
kutil::heap_allocator &g_kernel_heap = __g_kernel_heap_storage.value;

static kutil::no_construct<frame_allocator> __g_frame_allocator_storage;
frame_allocator &g_frame_allocator = __g_frame_allocator_storage.value;

static kutil::no_construct<vm_area_open> __g_kernel_heap_area_storage;
vm_area_open &g_kernel_heap_area = __g_kernel_heap_area_storage.value;

vm_area_buffers g_kernel_stacks {
	memory::kernel_max_stacks,
	vm_space::kernel_space(),
	vm_flags::write,
	memory::kernel_stack_pages};

vm_area_buffers g_kernel_buffers {
	memory::kernel_max_buffers,
	vm_space::kernel_space(),
	vm_flags::write,
	memory::kernel_buffer_pages};

void * operator new(size_t size)           { return g_kernel_heap.allocate(size); }
void * operator new [] (size_t size)       { return g_kernel_heap.allocate(size); }
void operator delete (void *p) noexcept    { return g_kernel_heap.free(p); }
void operator delete [] (void *p) noexcept { return g_kernel_heap.free(p); }

namespace kutil {
	void * kalloc(size_t size) { return g_kernel_heap.allocate(size); }
	void kfree(void *p) { return g_kernel_heap.free(p); }
}

void
memory_initialize_pre_ctors(args::header &kargs)
{
	using kernel::args::frame_block;

	new (&g_kernel_heap) kutil::heap_allocator {heap_start, kernel_max_heap};

	frame_block *blocks = reinterpret_cast<frame_block*>(memory::bitmap_start);
	new (&g_frame_allocator) frame_allocator {blocks, kargs.frame_block_count};

	// Mark all the things the bootloader allocated for us as used
	g_frame_allocator.used(
		reinterpret_cast<uintptr_t>(kargs.frame_blocks),
		kargs.frame_block_pages);

	g_frame_allocator.used(
		reinterpret_cast<uintptr_t>(kargs.pml4),
		kargs.table_pages);

	for (unsigned i = 0; i < kargs.num_modules; ++i) {
		const kernel::args::module &mod = kargs.modules[i];
		g_frame_allocator.used(
			reinterpret_cast<uintptr_t>(mod.location),
			memory::page_count(mod.size));
	}

	for (unsigned i = 0; i < kargs.num_programs; ++i) {
		const kernel::args::program &prog = kargs.programs[i];
		for (auto &sect : prog.sections) {
			if (!sect.size) continue;
			g_frame_allocator.used(
				sect.phys_addr,
				memory::page_count(sect.size));
		}
	}

	page_table *kpml4 = reinterpret_cast<page_table*>(kargs.pml4);
	process *kp = process::create_kernel_process(kpml4);
	vm_space &vm = kp->space();

	vm_area *heap = new (&g_kernel_heap_area)
		vm_area_open(kernel_max_heap, vm, vm_flags::write);

	vm.add(heap_start, heap);
}

void
memory_initialize_post_ctors(args::header &kargs)
{
	vm_space &vm = vm_space::kernel_space();
	vm.add(memory::stacks_start, &g_kernel_stacks);
	vm.add(memory::buffers_start, &g_kernel_buffers);

	g_frame_allocator.free(
		reinterpret_cast<uintptr_t>(kargs.page_tables),
		kargs.table_count);

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
setup_pat()
{
	uint64_t pat = rdmsr(msr::ia32_pat);
	pat = (pat & 0x00ffffffffffffffull) | (0x01ull << 56); // set PAT 7 to WC
	wrmsr(msr::ia32_pat, pat);
	log_mtrrs();
}


process *
load_simple_process(args::program &program)
{
	using kernel::args::section_flags;

	process *p = new process;
	vm_space &space = p->space();

	for (const auto &sect : program.sections) {
		vm_flags flags =
			(bitfield_has(sect.type, section_flags::execute) ? vm_flags::exec : vm_flags::none) |
			(bitfield_has(sect.type, section_flags::write) ? vm_flags::write : vm_flags::none);

		vm_area *vma = new vm_area_fixed(sect.size, flags);
		space.add(sect.virt_addr, vma);
		vma->commit(sect.phys_addr, 0, memory::page_count(sect.size));
	}

	uint64_t iopl = (3ull << 12);
	uintptr_t trampoline = reinterpret_cast<uintptr_t>(initialize_main_thread);

	thread *main = p->create_thread();
	main->add_thunk_user(program.entrypoint, trampoline, iopl);
	main->set_state(thread::state::ready);

	return p;
}

template <typename T>
inline T * push(uintptr_t &rsp, size_t size = sizeof(T)) {
	rsp -= size;
	T *p = reinterpret_cast<T*>(rsp);
	rsp &= ~(sizeof(uint64_t)-1); // Align the stack
	return p;
}

uintptr_t
initialize_main_user_stack()
{
	process &proc = process::current();
	thread &th = thread::current();
	TCB *tcb = th.tcb();

	const char message[] = "Hello from the kernel!";
	char *message_arg = push<char>(tcb->rsp3, sizeof(message));
	kutil::memcpy(message_arg, message, sizeof(message));

	extern args::framebuffer *fb;
	j6_init_framebuffer *fb_desc = push<j6_init_framebuffer>(tcb->rsp3);
	fb_desc->addr = fb ? reinterpret_cast<void*>(0x100000000) : nullptr;
	fb_desc->size = fb ? fb->size : 0;
	fb_desc->vertical = fb ? fb->vertical : 0;
	fb_desc->horizontal = fb ? fb->horizontal : 0;
	fb_desc->scanline = fb ? fb->scanline : 0;
	fb_desc->flags = 0;

	if (fb && fb->type == kernel::args::fb_type::bgr8)
		fb_desc->flags |= 1;

	unsigned n = 0;

	j6_init_value *initv = push<j6_init_value>(tcb->rsp3);
	initv->type = j6_init_handle_other;
	initv->handle.type = j6_object_type_system;
	initv->handle.handle = proc.add_handle(&system::get());
	++n;

	initv = push<j6_init_value>(tcb->rsp3);
	initv->type = j6_init_handle_self;
	initv->handle.type = j6_object_type_process;
	initv->handle.handle = proc.self_handle();
	++n;

	initv = push<j6_init_value>(tcb->rsp3);
	initv->type = j6_init_handle_self;
	initv->handle.type = j6_object_type_thread;
	initv->handle.handle = th.self_handle();
	++n;

	initv = push<j6_init_value>(tcb->rsp3);
	initv->type = j6_init_desc_framebuffer;
	initv->data = fb_desc;
	++n;

	uint64_t *initc = push<uint64_t>(tcb->rsp3);
	*initc = n;

	char **argv0 = push<char*>(tcb->rsp3);
	*argv0 = message_arg;

	uint64_t *argc = push<uint64_t>(tcb->rsp3);
	*argc = 1;

	th.clear_state(thread::state::loading);
	return tcb->rsp3;
}
