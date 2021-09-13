#include "kutil/assert.h"
#include "kutil/memory.h"
#include "frame_allocator.h"
#include "kernel_memory.h"
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

static_assert(sizeof(page_tree) == 66 * sizeof(uintptr_t));

static constexpr unsigned max_level = 5;
static constexpr unsigned bits_per_level = 6;

inline int level_shift(uint8_t level) { return level * bits_per_level + memory::frame_bits; }
inline uint64_t level_mask(uint8_t level) { return ~0x3full << level_shift(level); }
inline int index_for(uint64_t off, uint8_t level) { return (off >> level_shift(level)) & 0x3full; }

page_tree::page_tree(uint64_t base, uint8_t level) :
    m_base {base & level_mask(level)},
    m_level {level}
{
    kutil::memset(m_entries, 0, sizeof(m_entries));
}

page_tree::~page_tree()
{
    if (m_level) {
        for (auto &e : m_entries)
            delete e.child;
    } else {
        auto &fa = frame_allocator::get();
        for (auto &e : m_entries) {
            if (e.entry & 1)
                fa.free(e.entry & ~0xfffull, 1);
        }
    }
}

bool
page_tree::contains(uint64_t offset, uint8_t &index) const
{
    return (offset & level_mask(m_level)) == m_base;
}

bool
page_tree::find(const page_tree *root, uint64_t offset, uintptr_t &page)
{
    page_tree const *node = root;
    while (node) {
        uint8_t level = node->m_level;
        uint8_t index = 0;
        if (!node->contains(offset, index))
            return false;

        if (!level) {
            uintptr_t entry = node->m_entries[index].entry;
            page = entry & ~0xfffull;
            return (entry & 1); // bit 0 marks 'present'
        }

        node = node->m_entries[index].child;
    }

    return false;
}

bool
page_tree::find_or_add(page_tree * &root, uint64_t offset, uintptr_t &page)
{
    page_tree *level0 = nullptr;

    if (!root) {
        // There's no root yet, just make a level0 and make it
        // the root.
        level0 = new page_tree(offset, 0);
        root = level0;
    } else {
        // Find or insert an existing level0
        page_tree **parent = &root;
        page_tree *node = root;
        uint8_t parent_level = max_level + 1;

        while (node) {
            uint8_t level = node->m_level;
            uint8_t index = 0;
            if (!node->contains(offset, index)) {
                // We found a valid parent but the slot where this node should
                // go contains another node. Insert an intermediate parent of
                // this node and a new level0 into the parent.
                uint64_t other = node->m_base;
                uint8_t lcl = parent_level;
                while (index_for(offset, lcl) == index_for(other, lcl)) --lcl;

                page_tree *inter = new page_tree(offset, lcl);
                inter->m_entries[index_for(other, lcl)].child = node;
                *parent = inter;

                level0 = new page_tree(offset, 0);
                inter->m_entries[index_for(offset, lcl)].child = level0;
                break;
            }

            if (!level) {
                level0 = node;
                break;
            }

            parent = &node->m_entries[index].child;
            node = *parent;
        }

        kassert( node || parent, "Both node and parent were null in find_or_add");

        if (!node) {
            // We found a parent with an empty spot where this node should
            // be. Insert a new level0 there.
            level0 = new page_tree(offset, 0);
            *parent = level0;
        }
    }

    kassert(level0, "Got through find_or_add without a level0");
    uint8_t index = index_for(offset, 0);
    uint64_t &ent = level0->m_entries[index].entry;
    if (!(ent & 1)) {
        // No entry for this page exists, so make one
        if (!frame_allocator::get().allocate(1, &ent))
            return false;
        ent |= 1;
    }

    page = ent & ~0xfffull;
    return true;
}
