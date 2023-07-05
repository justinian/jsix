#include <string.h>

#include <arch/memory.h>

#include "assert.h"
#include "frame_allocator.h"
#include "page_tree.h"

// Page tree levels map the following parts of an offset. Note the xxx part of
// the offset but represent the bits of the actual sub-page virtual address.
// (Also note that level 0's entries are physical page addrs, the rest map
// other page_tree nodes)
//
// Level 0: 0000 0000 0003 fxxx    64 pages / 256 KiB
// Level 1: 0000 0000 00fc 0xxx    4K pages /  16 MiB -- 24-bit addressing
// Level 2: 0000 0000 3f00 0xxx  256K pages /   1 GiB
// Level 3: 0000 000f c000 0xxx   16M pages /  64 GiB -- 36-bit addressing
// Level 4: 0000 03f0 0000 0xxx    1G pages /   4 TiB
// Level 5: 0000 fc00 0000 0xxx   64G pages / 256 TiB -- 48-bit addressing
//
// Not supported until 5-level paging:
// Level 6: 003f 0000 0000 0xxx    4T pages /  16 PiB -- 54-bit addressing
// Level 7: 0fc0 0000 0000 0xxx  256T pages /   1 EiB -- 60-bit addressing

static_assert(sizeof(page_tree) == 67 * sizeof(uintptr_t));

bool
page_tree::find_or_add(page_tree * &root, uint64_t offset, uintptr_t &page)
{
    node_type * radix_root = root;
    uint64_t &ent = radix_tree::find_or_add(radix_root, offset);
    root = static_cast<page_tree*>(radix_root);

    if (!(ent & 1)) {
        // No entry for this page exists, so make one
        if (!frame_allocator::get().allocate(1, &ent))
            return false;
        ent |= 1;
    }

    page = ent & ~0xfffull;
    return true;
}

void
page_tree::add_existing(page_tree * &root, uint64_t offset, uintptr_t page)
{
    node_type * radix_root = root;
    uint64_t &ent = radix_tree::find_or_add(radix_root, offset);
    root = static_cast<page_tree*>(radix_root);

    kassert(!(ent & 1), "Replacing existing mapping in page_tree::add_existing");
    ent = page | 1;
}
