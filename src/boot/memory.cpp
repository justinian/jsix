#include <stddef.h>

#include <uefi/boot_services.h>
#include <uefi/runtime_services.h>
#include <uefi/types.h>

#include <bootproto/memory.h>

#include "console.h"
#include "error.h"
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
virtualize(paging::pager &pager, efi_mem_map &map, uefi::runtime_services *rs)
{
    pager.add_current_mappings();

    for (auto &desc : map)
        desc.virtual_start = desc.physical_start + bootproto::mem::linear_offset;

    pager.install();

    try_or_raise(
        rs->set_virtual_address_map(
            map.length, map.size, map.version, map.entries),
        L"Error setting virtual address map");
}


} // namespace boot
} // namespace memory
