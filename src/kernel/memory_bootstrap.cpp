#include <algorithm>
#include <utility>

#include "kutil/assert.h"
#include "kutil/heap_allocator.h"
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

static const unsigned ident_page_flags = 0xb;

kutil::vm_space g_kspace;
kutil::heap_allocator g_kernel_heap;
bool g_memory_initialized = false;

void * operator new(size_t size)           { return g_kernel_heap.allocate(size); }
void * operator new [] (size_t size)       { return g_kernel_heap.allocate(size); }
void operator delete (void *p) noexcept    { return g_kernel_heap.free(p); }
void operator delete [] (void *p) noexcept { return g_kernel_heap.free(p); }

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

	jsix_kernel = 0x80000000,
	jsix_data,
	jsix_initrd,
	jsix_scratch,

	jsix_max
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

struct memory_map
{
	memory_map(const void *efi_map, size_t map_length, size_t desc_length) :
		efi_map(efi_map), map_length(map_length), desc_length(desc_length) {}

	class iterator
	{
	public:
		iterator(const memory_map &map, efi_memory_descriptor const *item) :
			map(map), item(item) {}

		inline efi_memory_descriptor const * operator*() const { return item; }
		inline bool operator!=(const iterator &other) { return item != other.item; }
		inline iterator & operator++() {
			item = kutil::offset_pointer(item, map.desc_length);
			return *this;
		}

	private:
		const memory_map &map;
		efi_memory_descriptor const *item;
	};

	iterator begin() const {
		return iterator(*this, reinterpret_cast<efi_memory_descriptor const *>(efi_map));
	}

	iterator end() const {
		const void *end = kutil::offset_pointer(efi_map, map_length);
		return iterator(*this, reinterpret_cast<efi_memory_descriptor const *>(end));
	}

	const void *efi_map;
	size_t map_length;
	size_t desc_length;
};

class memory_bootstrap
{
public:
	memory_bootstrap(const void *memory_map, size_t map_length, size_t desc_length) :
		map(memory_map, map_length, desc_length) {}

	void add_free_frames(frame_allocator &fa) {
		for (auto *desc : map) {
			if (desc->type == efi_memory_type::loader_code ||
				desc->type == efi_memory_type::loader_data ||
				desc->type == efi_memory_type::boot_services_code ||
				desc->type == efi_memory_type::boot_services_data ||
				desc->type == efi_memory_type::available)
			{
				fa.free(desc->physical_start, desc->pages);
			}
		}
	}

	void add_used_frames(kutil::vm_space &vm) {
		for (auto *desc : map) {
			if (desc->type == efi_memory_type::jsix_data ||
				desc->type == efi_memory_type::jsix_initrd ||
				desc->type == efi_memory_type::jsix_kernel)
			{
				uintptr_t virt_addr = desc->physical_start + kernel_offset;
				vm.commit(virt_addr, desc->pages * frame_size);
			}
		}
	}

	void page_in_kernel(page_manager &pm, page_table *pml4) {
		for (auto *desc : map) {
			if (desc->type == efi_memory_type::jsix_kernel ||
				desc->type == efi_memory_type::jsix_data ||
				desc->type == efi_memory_type::jsix_initrd)
			{
				uintptr_t virt_addr = desc->physical_start + kernel_offset;
				pm.page_in(pml4, desc->physical_start, virt_addr, desc->pages);
			}

			if (desc->type == efi_memory_type::acpi_reclaim) {
				pm.page_in(pml4, desc->physical_start, desc->physical_start, desc->pages);
			}
		}

		// Put our new PML4 into CR3 to start using it
		page_manager::set_pml4(pml4);
		pm.m_kernel_pml4 = pml4;
	}

private:
	const memory_map map;
};

void
memory_initialize(uint16_t scratch_pages, const void *memory_map, size_t map_length, size_t desc_length)
{
	g_memory_initialized = false;

	// make sure the options we want in CR4 are set
	uint64_t cr4;
	__asm__ __volatile__ ( "mov %%cr4, %0" : "=r" (cr4) );
	cr4 |=
		0x000080 | // Enable global pages
		0x000200 | // Enable FXSAVE/FXRSTOR
		0x010000 | // Enable FSGSBASE
		0x020000 | // Enable PCIDs
		0;
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

	// Flags: 0  0 0 0 1  1 0 0 0  0 0 1 1 = 0x0183
	//        |   IGN  |  | | | |  | | | +- Present
	//        |        |  | | | |  | | +--- Writeable
	//        |        |  | | | |  | +----- Supervisor only
	//        |        |  | | | |  +------- PWT (determining memory type for page)
	//        |        |  | | | +---------- PCD (determining memory type for page)
	//        |        |  | | +------------ Accessed flag (not accessed yet)
	//        |        |  | +-------------- Dirty (not dirtied yet)
	//        |        |  +---------------- Page size (1GiB page)
	//        |        +------------------- Global
	//        +---------------------------- PAT (determining memory type for page)
	for (int i=0; i<512; ++i)
		id_pdp->entries[i] = (static_cast<uintptr_t>(i) << 30) | 0x0183;

	// Flags: 0 0 0 0  0 0 0 0  0 0 1 1 = 0x0003
	//        IGNORED  | | | |  | | | +- Present
	//                 | | | |  | | +--- Writeable
	//                 | | | |  | +----- Supervisor only
	//                 | | | |  +------- PWT (determining memory type for pdpt)
	//                 | | | +---------- PCD (determining memory type for pdpt)
	//                 | | +------------ Accessed flag (not accessed yet)
	//                 | +-------------- Ignored
	//                 +---------------- Reserved 0
	id_pml4->entries[511] = reinterpret_cast<uintptr_t>(id_pdp) | 0x003;

	// Make sure the page table is finished updating before we write to memory
	__sync_synchronize();
	io_wait();

	uintptr_t scratch_virt = scratch_phys + page_offset;
	memory_bootstrap bootstrap {memory_map, map_length, desc_length};

	// Now tell the frame allocator what's free
	frame_allocator *fa = new (&g_frame_allocator) frame_allocator;
	bootstrap.add_free_frames(*fa);

	new (&g_kernel_heap) kutil::heap_allocator(heap_start, kernel_max_heap);
	new (&g_kspace) kutil::vm_space(kernel_offset, (page_offset-kernel_offset), g_kernel_heap);
	bootstrap.add_used_frames(g_kspace);

	// Create the page manager
	page_manager *pm = new (&g_page_manager) page_manager(*fa);

	// Give the frame_allocator back the rest of the scratch pages
	fa->free(scratch_phys + (3 * frame_size), scratch_pages - 3);

	// Finally, build an acutal set of kernel page tables where we'll only add
	// what the kernel actually has mapped, but making everything writable
	// (especially the page tables themselves)
	page_table *pml4 = &tables[2];
	pml4 = kutil::offset_pointer(pml4, page_offset);

	kutil::memset(pml4, 0, sizeof(page_table));
	pml4->entries[511] = reinterpret_cast<uintptr_t>(id_pdp) | 0x003;

	bootstrap.page_in_kernel(*pm, pml4);

	// Reclaim the old PML4
	fa->free(scratch_phys, 1);
}
