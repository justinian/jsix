#include <utility>

#include "kernel_args.h"

#include "kutil/assert.h"
#include "kutil/heap_allocator.h"
#include "kutil/no_construct.h"

#include "frame_allocator.h"
#include "io.h"
#include "log.h"
#include "msr.h"
#include "objects/process.h"
#include "objects/vm_area.h"
#include "vm_space.h"

using memory::frame_size;
using memory::heap_start;
using memory::kernel_max_heap;
using memory::kernel_offset;
using memory::heap_start;
using memory::page_offset;
using memory::pml4e_kernel;
using memory::pml4e_offset;
using memory::table_entries;

using namespace kernel;


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

/*
void walk_page_table(
	page_table *table,
	page_table::level level,
	uintptr_t &current_start,
	size_t &current_bytes,
	vm_area &karea)
{
	constexpr size_t huge_page_size = (1ull<<30);
	constexpr size_t large_page_size = (1ull<<21);

	for (unsigned i = 0; i < table_entries; ++i) {
		page_table *next = table->get(i);
		if (!next) {
			if (current_bytes)
				karea.commit(current_start, current_bytes);
			current_start = 0;
			current_bytes = 0;
			continue;
		} else if (table->is_page(level, i)) {
			if (!current_bytes)
				current_start = reinterpret_cast<uintptr_t>(next);
			current_bytes +=
				(level == page_table::level::pt
					? frame_size
					: level == page_table::level::pd
						? large_page_size
						: huge_page_size);
		} else {
			page_table::level deeper =
				static_cast<page_table::level>(
					static_cast<unsigned>(level) + 1);

			walk_page_table(
				next, deeper, current_start, current_bytes, kspace);
		}
	}
}
*/

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


void
memory_initialize_pre_ctors(args::header *kargs)
{
	new (&g_kernel_heap) kutil::heap_allocator {heap_start, kernel_max_heap};
	new (&g_frame_allocator) frame_allocator;

	args::mem_entry *entries = kargs->mem_map;
	const size_t count = kargs->map_count;
	for (unsigned i = 0; i < count; ++i) {
		// TODO: use entry attributes
		// TODO: copy anything we need from "pending" memory and free it
		args::mem_entry &e = entries[i];
		if (e.type == args::mem_type::free)
			g_frame_allocator.free(e.start, e.pages);
	}

	page_table *kpml4 = reinterpret_cast<page_table*>(kargs->pml4);
	process *kp = process::create_kernel_process(kpml4);
	vm_space &vm = kp->space();

	vm_area *heap = new (&g_kernel_heap_area)
		vm_area_open(memory::kernel_max_heap, vm, vm_flags::write);

	vm.add(memory::heap_start, heap);
}

void
memory_initialize_post_ctors(args::header *kargs)
{
	/*
	uintptr_t current_start = 0;
	size_t current_bytes = 0;

	// TODO: Should we exclude the top of this area? (eg, buffers, stacks, etc)
	page_table *kpml4 = reinterpret_cast<page_table*>(kargs->pml4);
	for (unsigned i = pml4e_kernel; i < pml4e_offset; ++i) {
		page_table *pdp = kpml4->get(i);
		kassert(pdp, "Bootloader did not create all kernelspace PDs");

		walk_page_table(
			pdp, page_table::level::pdp,
			current_start, current_bytes,
			g_kernel_space);
	}

	if (current_bytes)
		g_kernel_space.commit(current_start, current_bytes);
	*/
	vm_space &vm = vm_space::kernel_space();
	vm.add(memory::stacks_start, &g_kernel_stacks);
	vm.add(memory::buffers_start, &g_kernel_buffers);

	g_frame_allocator.free(
		reinterpret_cast<uintptr_t>(kargs->page_tables),
		kargs->table_count);

}
