#include <algorithm>
#include <utility>
#include "kutil/assert.h"
#include "kutil/frame_allocator.h"
#include "kutil/heap_manager.h"
#include "io.h"
#include "log.h"
#include "page_manager.h"

using kutil::frame_block;
using kutil::frame_block_flags;
using kutil::frame_block_list;

static const unsigned ident_page_flags = 0xb;
static const size_t page_size = page_manager::page_size;

extern kutil::frame_allocator g_frame_allocator;

kutil::heap_manager g_kernel_heap_manager;

void * mm_grow_callback(void *next, size_t length)
{
	kassert(length % page_manager::page_size == 0,
			"Heap manager requested a fractional page.");

	size_t pages = length / page_manager::page_size;
	log::info(logs::memory, "Heap manager growing heap by %d pages.", pages);
	g_page_manager.map_pages(reinterpret_cast<uintptr_t>(next), pages);
	return next;
}


namespace {
	// Page-by-page initial allocator for the initial frame_block allocator
	struct page_consumer
	{
		page_consumer(uintptr_t start, unsigned count, unsigned used = 0) :
			current(start + used * page_size),
			used(used),
			max(count) {}

		void * get_page() {
			kassert(used++ < max, "page_consumer ran out of pages");
			void *retval = reinterpret_cast<void *>(current);
			current += page_size;
			return retval;
		}

		void * operator()(size_t size) {
			kassert(size == page_size, "page_consumer used with non-page size!");
			return get_page();
		}

		unsigned left() const { return max - used; }

		uintptr_t current;
		unsigned used, max;
	};

	using block_allocator =
		kutil::slab_allocator<kutil::frame_block, page_consumer &>;
}

enum class efi_memory_type : uint32_t
{
	reserved,
	loader_code,
	loader_data,
	boot_services_code,
	boot_services_data,
	runtime_services_code,
	runtime_services_data,
	available,
	unusable,
	acpi_reclaim,
	acpi_nvs,
	mmio,
	mmio_port,
	pal_code,
	persistent,

	efi_max,

	popcorn_kernel = 0x80000000,
	popcorn_data,
	popcorn_initrd,
	popcorn_scratch,

	popcorn_max
};

struct efi_memory_descriptor
{
	efi_memory_type type;
	uint32_t pad;
	uint64_t physical_start;
	uint64_t virtual_start;
	uint64_t pages;
	uint64_t flags;
};

static const efi_memory_descriptor *
desc_incr(const efi_memory_descriptor *d, size_t desc_length)
{
	return reinterpret_cast<const efi_memory_descriptor *>(
			reinterpret_cast<const uint8_t *>(d) + desc_length);
}

void
gather_block_lists(
		block_allocator &allocator,
		frame_block_list &used,
		frame_block_list &free,
		const void *memory_map,
		size_t map_length,
		size_t desc_length)
{
	efi_memory_descriptor const *desc = reinterpret_cast<efi_memory_descriptor const *>(memory_map);
	efi_memory_descriptor const *end = desc_incr(desc, map_length);

	while (desc < end) {
		auto *block = allocator.pop();
		block->address = desc->physical_start;
		block->count = desc->pages;
		bool block_used;

		switch (desc->type) {
		case efi_memory_type::loader_code:
		case efi_memory_type::loader_data:
			block_used = true;
			block->flags = frame_block_flags::pending_free;
			break;

		case efi_memory_type::boot_services_code:
		case efi_memory_type::boot_services_data:
		case efi_memory_type::available:
			block_used = false;
			break;

		case efi_memory_type::acpi_reclaim:
			block_used = true;
			block->flags =
				frame_block_flags::acpi_wait |
				frame_block_flags::map_ident;
			break;

		case efi_memory_type::persistent:
			block_used = false;
			block->flags = frame_block_flags::nonvolatile;
			break;

		case efi_memory_type::popcorn_kernel:
			block_used = true;
			block->flags = frame_block_flags::map_kernel;
			break;

		case efi_memory_type::popcorn_data:
		case efi_memory_type::popcorn_initrd:
			block_used = true;
			block->flags =
				frame_block_flags::pending_free |
				frame_block_flags::map_kernel;
			break;

		case efi_memory_type::popcorn_scratch:
			block_used = true;
			block->flags = frame_block_flags::map_offset;
			break;

		default:
			block_used = true;
			block->flags = frame_block_flags::permanent;
			break;
		}

		if (block_used) 
			used.push_back(block);
		else
			free.push_back(block);

		desc = desc_incr(desc, desc_length);
	}
}

static unsigned
check_needs_page_ident(page_table *table, unsigned index, page_table **free_pages)
{
	if ((table->entries[index] & 0x1) == 1) return 0;

	kassert(*free_pages, "check_needs_page_ident needed to allocate but had no free pages");

	page_table *new_table = (*free_pages)++;
	for (int i=0; i<512; ++i) new_table->entries[i] = 0;
	table->entries[index] = reinterpret_cast<uint64_t>(new_table) | ident_page_flags;
	return 1;
}

static unsigned
page_in_ident(
		page_table *pml4,
		uint64_t phys_addr,
		uint64_t virt_addr,
		uint64_t count,
		page_table *free_pages)
{
	page_table_indices idx{virt_addr};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	unsigned pages_consumed = 0;
	for (; idx[0] < 512; idx[0] += 1) {
		pages_consumed += check_needs_page_ident(tables[0], idx[0], &free_pages);
		tables[1] = reinterpret_cast<page_table *>(
				tables[0]->entries[idx[0]] & ~0xfffull);

		for (; idx[1] < 512; idx[1] += 1, idx[2] = 0, idx[3] = 0) {
			pages_consumed += check_needs_page_ident(tables[1], idx[1], &free_pages);
			tables[2] = reinterpret_cast<page_table *>(
					tables[1]->entries[idx[1]] & ~0xfffull);

			for (; idx[2] < 512; idx[2] += 1, idx[3] = 0) {
				if (idx[3] == 0 &&
					count >= 512 &&
					tables[2]->get(idx[2]) == nullptr) {
					// Do a 2MiB page instead
					tables[2]->entries[idx[2]] = phys_addr | 0x80 | ident_page_flags;

					phys_addr += page_size * 512;
					count -= 512;
					if (count == 0) return pages_consumed;
					continue;
				}

				pages_consumed += check_needs_page_ident(tables[2], idx[2], &free_pages);
				tables[3] = reinterpret_cast<page_table *>(
						tables[2]->entries[idx[2]] & ~0xfffull);

				for (; idx[3] < 512; idx[3] += 1) {
					tables[3]->entries[idx[3]] = phys_addr | ident_page_flags;
					phys_addr += page_size;
					if (--count == 0) return pages_consumed;
				}
			}
		}
	}

	kassert(0, "Ran to end of page_in_ident");
	return 0; // Cannot reach
}

void
memory_initialize(uint16_t scratch_pages, const void *memory_map, size_t map_length, size_t desc_length)
{
	// make sure the options we want in CR4 are set
	uint64_t cr4;
	__asm__ __volatile__ ( "mov %%cr4, %0" : "=r" (cr4) );
	cr4 |= 0x00080; // Enable global pages
	cr4 |= 0x00200; // Enable FXSAVE/FXRSTOR
	cr4 |= 0x20000; // Enable PCIDs
	__asm__ __volatile__ ( "mov %0, %%cr4" :: "r" (cr4) );

	// The bootloader reserved "scratch_pages" pages for page tables and
	// scratch space, which we'll use to bootstrap.  The first one is the
	// already-installed PML4, so grab it from CR3.
	uint64_t scratch_phys;
	__asm__ __volatile__ ( "mov %%cr3, %0" : "=r" (scratch_phys) );
	scratch_phys &= ~0xfffull;

	// The tables are ident-mapped currently, so the cr3 physical address works. But let's
	// get them into the offset-mapped area asap.
	page_table *tables = reinterpret_cast<page_table *>(scratch_phys);
	uintptr_t scratch_virt = scratch_phys + page_manager::page_offset;

	uint64_t used_pages = 1; // starts with PML4
	used_pages += page_in_ident(
			&tables[0],
			scratch_phys,
			scratch_virt,
			scratch_pages,
			tables + used_pages);

	// Make sure the page table is finished updating before we write to memory
	__sync_synchronize();
	io_wait();

	// We now have pages starting at "scratch_virt" to bootstrap ourselves. Start by
	// taking inventory of free pages.
	page_consumer allocator(scratch_virt, scratch_pages, used_pages);

	block_allocator block_slab(page_size, allocator);
	frame_block_list used;
	frame_block_list free;

	gather_block_lists(block_slab, used, free, memory_map, map_length, desc_length);
	block_slab.allocate(); // Make sure we have extra

	// Now go back through these lists and consolidate
	block_slab.append(frame_block::consolidate(free));
	block_slab.append(frame_block::consolidate(used));

	// Finally, build an acutal set of kernel page tables that just contains
	// what the kernel actually has mapped, but making everything writable
	// (especially the page tables themselves)
	page_table *pml4 = reinterpret_cast<page_table *>(allocator.get_page());
	for (int i=0; i<512; ++i) pml4->entries[i] = 0;

	kutil::frame_allocator *fa =
		new (&g_frame_allocator) kutil::frame_allocator(std::move(block_slab));
	page_manager *pm = new (&g_page_manager) page_manager(*fa);

	// Give the rest to the page_manager's cache for use in page_in
	pm->free_table_pages(
			reinterpret_cast<void *>(allocator.current),
			allocator.left());

	uintptr_t heap_start = page_manager::high_offset;

	for (auto *block : used) {
		uintptr_t virt_addr = 0;

		switch (block->flags & frame_block_flags::map_mask) {
			case frame_block_flags::map_ident:
				virt_addr = block->address;
				break;

			case frame_block_flags::map_kernel:
				virt_addr = block->address + page_manager::high_offset;
				heap_start = std::max(heap_start,
						virt_addr + block->count * page_size);
				break;

			case frame_block_flags::map_offset:
				virt_addr = block->address + page_manager::page_offset;
				break;

			default:
				break;
		}

		block->flags -= frame_block_flags::map_mask;
		if (virt_addr)
			pm->page_in(pml4, block->address, virt_addr, block->count);
	}

	fa->init(std::move(free), std::move(used));

	// Put our new PML4 into CR3 to start using it
	page_manager::set_pml4(pml4);
	pm->m_kernel_pml4 = pml4;

	// Set the heap manager
	new (&g_kernel_heap_manager) kutil::heap_manager(
			reinterpret_cast<void *>(heap_start),
			mm_grow_callback);
	kutil::setup::set_heap(&g_kernel_heap_manager);
}
