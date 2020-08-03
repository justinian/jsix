#include <algorithm>
#include <utility>

#include "kernel_args.h"

#include "kutil/assert.h"
#include "kutil/heap_allocator.h"
#include "kutil/no_construct.h"
#include "kutil/vm_space.h"

#include "frame_allocator.h"
#include "io.h"
#include "log.h"
#include "page_manager.h"

using memory::frame_size;
using memory::heap_start;
using memory::kernel_max_heap;
using memory::kernel_offset;
using memory::page_offset;
using memory::pml4e_kernel;
using memory::pml4e_offset;
using memory::table_entries;

using namespace kernel;

kutil::vm_space g_kernel_space {kernel_offset, (page_offset-kernel_offset)};


// These objects are initialized _before_ global constructors are called,
// so we don't want them to have global constructors at all, lest they
// overwrite the previous initialization.
static kutil::no_construct<kutil::heap_allocator> __g_kernel_heap_storage;
kutil::heap_allocator &g_kernel_heap = __g_kernel_heap_storage.value;

static kutil::no_construct<page_manager> __g_page_manager_storage;
page_manager &g_page_manager = __g_page_manager_storage.value;

static kutil::no_construct<frame_allocator> __g_frame_allocator_storage;
frame_allocator &g_frame_allocator = __g_frame_allocator_storage.value;

void * operator new(size_t size)           { return g_kernel_heap.allocate(size); }
void * operator new [] (size_t size)       { return g_kernel_heap.allocate(size); }
void operator delete (void *p) noexcept    { return g_kernel_heap.free(p); }
void operator delete [] (void *p) noexcept { return g_kernel_heap.free(p); }

namespace kutil {
void * kalloc(size_t size) { return g_kernel_heap.allocate(size); }
void kfree(void *p) { return g_kernel_heap.free(p); }
}

void walk_page_table(
	page_table *table,
	page_table::level level,
	uintptr_t &current_start,
	size_t &current_bytes,
	kutil::vm_space &kspace)
{
	constexpr size_t huge_page_size = (1ull<<30);
	constexpr size_t large_page_size = (1ull<<21);

	for (unsigned i = 0; i < table_entries; ++i) {
		page_table *next = table->get(i);
		if (!next) {
			if (current_bytes)
				kspace.commit(current_start, current_bytes);
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

void
memory_initialize_pre_ctors(args::header *kargs)
{
	args::mem_entry *entries = kargs->mem_map;
	size_t entry_count = kargs->num_map_entries;
	page_table *kpml4 = reinterpret_cast<page_table*>(kargs->pml4);

	new (&g_kernel_heap) kutil::heap_allocator {heap_start, kernel_max_heap};

	new (&g_frame_allocator) frame_allocator;
	for (unsigned i = 0; i < entry_count; ++i) {
		// TODO: use entry attributes
		args::mem_entry &e = entries[i];
		if (e.type == args::mem_type::free)
			g_frame_allocator.free(e.start, e.pages);
	}

	// Create the page manager
	new (&g_page_manager) page_manager {g_frame_allocator, kpml4};
}

void
memory_initialize_post_ctors(args::header *kargs)
{
	uintptr_t current_start = 0;
	size_t current_bytes = 0;

	// TODO: Should we exclude the top of this area? (eg, buffers, stacks, etc)
	page_table *kpml4 = reinterpret_cast<page_table*>(kargs->pml4);
	for (unsigned i = pml4e_kernel; i < pml4e_offset; ++i) {
		page_table *pdp = kpml4->get(i);

		if (!pdp) {
			if (current_bytes)
				g_kernel_space.commit(current_start, current_bytes);
			current_start = 0;
			current_bytes = 0;
			continue;
		}

		walk_page_table(
			pdp, page_table::level::pdp,
			current_start, current_bytes,
			g_kernel_space);
	}

	g_frame_allocator.free(
		reinterpret_cast<uintptr_t>(kargs->page_table_cache),
		kargs->num_free_tables);
}
