#include <string.h>
#include <util/no_construct.h>

#include "assert.h"
#include "cpu.h"
#include "logger.h"
#include "memory.h"
#include "objects/vm_area.h"
#include "tss.h"

// The BSP's TSS is initialized _before_ global constructors are called,
// so we don't want it to have a global constructor, lest it overwrite
// the previous initialization.
static util::no_construct<TSS> __g_bsp_tss_storage;
TSS &g_bsp_tss = __g_bsp_tss_storage.value;


TSS::TSS()
{
    memset(this, 0, sizeof(TSS));
    m_iomap_offset = sizeof(TSS);
}

TSS &
TSS::current()
{
    return *current_cpu().tss;
}

uintptr_t &
TSS::ring_stack(unsigned ring)
{
    kassert(ring < 3, "Bad ring passed to TSS::ring_stack.");
    return m_rsp[ring];
}

uintptr_t &
TSS::ist_stack(unsigned ist)
{
    kassert(ist > 0 && ist < 7, "Bad ist passed to TSS::ist_stack.");
    return m_ist[ist];
}

void
TSS::create_ist_stacks(uint8_t ist_entries)
{
    extern obj::vm_area_guarded &g_kernel_stacks;
    using mem::frame_size;
    using mem::kernel_stack_pages;
    constexpr size_t stack_bytes = kernel_stack_pages * frame_size;

    for (unsigned ist = 1; ist < 8; ++ist) {
        if (!(ist_entries & (1 << ist))) continue;

        // Two zero entries at the top for the null frame
        uintptr_t stack_bottom = g_kernel_stacks.get_section();
        uintptr_t stack_top = stack_bottom + stack_bytes - 2 * sizeof(uintptr_t);

        log::verbose(logs::memory, "Created IST stack at %016lx size 0x%lx",
                stack_bottom, stack_bytes);

        // Pre-realize these stacks, they're no good if they page fault
        for (unsigned i = 0; i < kernel_stack_pages; ++i)
            *reinterpret_cast<uint64_t*>(stack_bottom + i * frame_size) = 0;

        ist_stack(ist) = stack_top;
    }
}
