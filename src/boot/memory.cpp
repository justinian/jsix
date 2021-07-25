#include <stddef.h>

#include <uefi/boot_services.h>
#include <uefi/runtime_services.h>
#include <uefi/types.h>
#include <kutil/no_construct.h>

#include "console.h"
#include "error.h"
#include "kernel_memory.h"
#include "memory.h"
#include "memory_map.h"
#include "paging.h"
#include "status.h"

namespace boot {
namespace memory {

size_t fixup_pointer_index = 0;
void **fixup_pointers[64];

void
update_marked_addresses(uefi::event, void *context)
{
	uefi::runtime_services *rs =
		reinterpret_cast<uefi::runtime_services*>(context);

	for (size_t i = 0; i < fixup_pointer_index; ++i) {
		if (fixup_pointers[i])
			rs->convert_pointer(0, fixup_pointers[i]);
	}
}

void
init_pointer_fixup(uefi::boot_services *bs, uefi::runtime_services *rs)
{
	status_line status(L"Initializing pointer virtualization event");

	uefi::event event;
	bs->set_mem(&fixup_pointers, sizeof(fixup_pointers), 0);
	fixup_pointer_index = 0;

	try_or_raise(
		bs->create_event(
			uefi::evt::signal_virtual_address_change,
			uefi::tpl::callback,
			(uefi::event_notify)&update_marked_addresses,
			rs,
			&event),
		L"Error creating memory virtualization event");
}

void
mark_pointer_fixup(void **p)
{
	fixup_pointers[fixup_pointer_index++] = p;
}

void
virtualize(void *pml4, efi_mem_map &map, uefi::runtime_services *rs)
{
	paging::add_current_mappings(reinterpret_cast<paging::page_table*>(pml4));

	for (auto &desc : map)
		desc.virtual_start = desc.physical_start + ::memory::page_offset;

	// Write our new PML4 pointer to CR3
	asm volatile ( "mov %0, %%cr3" :: "r" (pml4) );
	__sync_synchronize();

	try_or_raise(
		rs->set_virtual_address_map(
			map.length, map.size, map.version, map.entries),
		L"Error setting virtual address map");
}


} // namespace boot
} // namespace memory
