#include <algorithm>
#include <utility>
#include "kutil/address_manager.h"
#include "kutil/assert.h"
#include "kutil/frame_allocator.h"
#include "kutil/heap_manager.h"
#include "io.h"
#include "log.h"
#include "page_manager.h"

using kutil::frame_block;
using kutil::frame_block_flags;
using kutil::frame_block_list;
using memory::frame_size;
using memory::kernel_offset;
using memory::page_offset;

static const unsigned ident_page_flags = 0xb;

kutil::frame_allocator g_frame_allocator;
kutil::address_manager g_kernel_address_manager;
kutil::heap_manager g_kernel_heap_manager;

void * mm_grow_callback(size_t length)
{
	kassert(length % frame_size == 0,
			"Heap manager requested a fractional page.");

	size_t pages = length / frame_size;
	log::info(logs::memory, "Heap manager growing heap by %d pages.", pages);

	uintptr_t addr = g_kernel_address_manager.allocate(length);
	g_page_manager.map_pages(addr, pages);

	return reinterpret_cast<void *>(addr);
}


namespace {
	// Page-by-page initial allocator for the initial frame_block allocator
	struct page_consumer
	{
		page_consumer(uintptr_t start, unsigned count, unsigned used = 0) :
			current(start + used * frame_size),
			used(used),
			max(count) {}

		void * get_page() {
			kassert(used++ < max, "page_consumer ran out of pages");
			void *retval = reinterpret_cast<void *>(current);
			current += frame_size;
			return retval;
		}

		void * operator()(size_t size) {
			kassert(size == frame_size, "page_consumer used with non-page size!");
			return get_page();
		}

		unsigned left() const { return max - used; }

		uintptr_t current;
		unsigned used, max;
	};

	using block_allocator =
		kutil::slab_allocator<kutil::frame_block, page_consumer &>;
	using region_allocator =
		kutil::slab_allocator<kutil::buddy_region, page_consumer &>;
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
			block->flags =
				frame_block_flags::permanent |
				frame_block_flags::map_kernel;
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

	page_table *id_pml4 = &tables[0];
	page_table *id_pdp = &tables[1];
	for (int i=0; i<512; ++i)
		id_pdp->entries[i] = (static_cast<uintptr_t>(i) << 30) | 0x18b;
	id_pml4->entries[511] = reinterpret_cast<uintptr_t>(id_pdp) | 0x10b;

	// Make sure the page table is finished updating before we write to memory
	__sync_synchronize();
	io_wait();

	// We now have pages starting at "scratch_virt" to bootstrap ourselves. Start by
	// taking inventory of free pages.
	uintptr_t scratch_virt = scratch_phys + page_offset;
	uint64_t used_pages = 2; // starts with PML4 + offset PDP
	page_consumer allocator(scratch_virt, scratch_pages, used_pages);

	block_allocator block_slab(frame_size, allocator);
	frame_block_list used;
	frame_block_list free;

	gather_block_lists(block_slab, used, free, memory_map, map_length, desc_length);
	block_slab.allocate(); // Make sure we have extra

	// Now go back through these lists and consolidate
	block_slab.append(frame_block::consolidate(free));

	region_allocator region_slab(frame_size, allocator);
	region_slab.allocate(); // Allocate some buddy regions for the address_manager

	kutil::address_manager *am =
		new (&g_kernel_address_manager) kutil::address_manager(std::move(region_slab));

	am->add_regions(kernel_offset, page_offset - kernel_offset);

	// Finally, build an acutal set of kernel page tables that just contains
	// what the kernel actually has mapped, but making everything writable
	// (especially the page tables themselves)
	page_table *pml4 = reinterpret_cast<page_table *>(allocator.get_page());
	kutil::memset(pml4, 0, sizeof(page_table));
	pml4->entries[511] = reinterpret_cast<uintptr_t>(id_pdp) | 0x10b;

	kutil::frame_allocator *fa =
		new (&g_frame_allocator) kutil::frame_allocator(std::move(block_slab));
	page_manager *pm = new (&g_page_manager) page_manager(*fa, *am);

	// Give the rest to the page_manager's cache for use in page_in
	pm->free_table_pages(
			reinterpret_cast<void *>(allocator.current),
			allocator.left());

	for (auto *block : used) {
		uintptr_t virt_addr = 0;

		switch (block->flags & frame_block_flags::map_mask) {
			case frame_block_flags::map_ident:
				virt_addr = block->address;
				break;

			case frame_block_flags::map_kernel:
				virt_addr = block->address + kernel_offset;
				if (block->flags && frame_block_flags::permanent)
					am->mark_permanent(virt_addr, block->count * frame_size);
				else
					am->mark(virt_addr, block->count * frame_size);
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

	// Give the old pml4 back to the page_manager to recycle
	pm->free_table_pages(reinterpret_cast<void *>(scratch_virt), 1);

	// Set the heap manager
	new (&g_kernel_heap_manager) kutil::heap_manager(mm_grow_callback);
	kutil::setup::set_heap(&g_kernel_heap_manager);
}
