#include "kutil/assert.h"
#include "kutil/memory.h"
#include "frame_allocator.h"
#include "page_tree.h"

// Page tree levels map the following parts of a pagewise offset:
// (Note that a level 0's entries are physical page addrs, the rest
// map other page_tree nodes)
//
// Level 0: 0000000003f    64 pages / 256 KiB 
// Level 1: 00000000fc0    4K pages /  16 MiB
// Level 2: 0000003f000  256K pages /   1 GiB
// Level 3: 00000fc0000   16M pages /  64 GiB
// Level 4: 0003f000000    1G pages /   4 TiB
// Level 5: 00fc0000000   64G pages / 256 TiB
// Level 6: 3f000000000    4T pages /  16 PiB -- Not supported until 5-level paging

static constexpr unsigned max_level = 5;
static constexpr unsigned bits_per_level = 6;

inline uint64_t to_word(uint64_t base, uint64_t level, uint64_t flags = 0) {
    // Clear out the non-appropriate bits for this level
    base &= (~0x3full << (level*bits_per_level));

    return
        (base & 0x3ffffffffff) |
        ((level & 0x7) << 42) |
        ((flags & 0x7ffff) << 45);
}

inline uint64_t to_base(uint64_t word) {
    return word & 0x3ffffffffff;
}

inline uint64_t to_level(uint64_t word) {
    return (word >> 42) & 0x3f;
}

inline uint64_t to_flags(uint64_t word) {
    return (word >> 45);
}

inline bool contains(uint64_t page_off, uint64_t word, uint8_t &index) {
    uint64_t base = to_base(word);
    uint64_t bits = to_level(word) * bits_per_level;
    index = (page_off >> bits) & 0x3f;
    return (page_off & (~0x3full << bits)) == base;
}

inline uint64_t index_for(uint64_t page_off, uint8_t level) {
    return (page_off >> (level*bits_per_level))  & 0x3f;
}

page_tree::page_tree(uint64_t base, uint8_t level) :
    m_base {to_word(base, level)}
{
    kutil::memset(m_entries, 0, sizeof(m_entries));
}

bool
page_tree::find(const page_tree *root, uint64_t offset, uintptr_t &page)
{
    uint64_t page_off = offset >> 12; // change to pagewise offset
    page_tree const *node = root;
    while (node) {
        uint8_t level = to_level(node->m_base);
        uint8_t index = 0;
        if (!contains(page_off, node->m_base, index))
            return false;

        if (!level) {
            uintptr_t entry = node->m_entries[index].entry;
            page = entry & ~1ull; // bit 0 marks 'present'
            return (entry & 1);
        }

        node = node->m_entries[index].child;
    }

    return false;
}

bool
page_tree::find_or_add(page_tree * &root, uint64_t offset, uintptr_t &page)
{
    uint64_t page_off = offset >> 12; // change to pagewise offset
    page_tree *level0 = nullptr;

    if (!root) {
        // There's no root yet, just make a level0 and make it
        // the root.
        level0 = new page_tree(page_off, 0);
        root = level0;
    } else {
        // Find or insert an existing level0
        page_tree **parent = &root;
        page_tree *node = root;
        uint8_t parent_level = max_level + 1;

        while (node) {
            uint8_t level = to_level(node->m_base);
            uint8_t index = 0;
            if (!contains(page_off, node->m_base, index)) {
                // We found a valid parent but the slot where this node should
                // go contains another node. Insert an intermediate parent of
                // this node and a new level0 into the parent.
                uint64_t other = to_base(node->m_base);
                uint8_t lcl = parent_level;
                while (index_for(page_off, lcl) == index_for(other, lcl))
                    --lcl;

                page_tree *inter = new page_tree(page_off, lcl);
                inter->m_entries[index_for(other, lcl)].child = node;
                *parent = inter;

                level0 = new page_tree(page_off, 0);
                inter->m_entries[index_for(page_off, lcl)].child = level0;
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
            level0 = new page_tree(page_off, 0);
            *parent = level0;
        }
    }

    kassert(level0, "Got through find_or_add without a level0");
    uint8_t index = index_for(page_off, 0);
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
