#include <bootproto/kernel.h>

#include "assert.h"
#include "frame_allocator.h"
#include "log.h"
#include "memory.h"

using mem::frame_size;


frame_allocator &
frame_allocator::get()
{
    extern frame_allocator &g_frame_allocator;
    return g_frame_allocator;
}

frame_allocator::frame_allocator(bootproto::frame_block *frames, size_t count) :
    m_blocks {frames},
    m_count {count}
{
}

inline unsigned
bsf(uint64_t v)
{
    asm ("tzcntq %q0, %q1" : "=r"(v) : "0"(v) : "cc");
    return v;
}

size_t
frame_allocator::allocate(size_t count, uintptr_t *address)
{
    util::scoped_lock lock {m_lock};

    for (long i = m_count - 1; i >= 0; --i) {
        frame_block &block = m_blocks[i];

        if (!block.map1)
            continue;

        // Tree walk to find the first available page
        unsigned o1 = bsf(block.map1);

        uint64_t m2 = block.map2[o1];
        unsigned o2 = bsf(m2);

        uint64_t m3 = block.bitmap[(o1 << 6) + o2];
        unsigned o3 = bsf(m3);

        unsigned frame = (o1 << 12) + (o2 << 6) + o3;

        // See how many contiguous pages are here
        unsigned n = bsf(~m3 >> o3);
        if (n > count)
            n = count;

        *address = block.base + frame * frame_size;

        // Clear the bits to mark these pages allocated
        m3 &= ~(((1ull << n) - 1) << o3);
        block.bitmap[(o1 << 6) + o2] = m3;
        if (!m3) {
            // if that was it for this group, clear the next level bit
            m2 &= ~(1ull << o2);
            block.map2[o1] = m2;

            if (!m2) {
                // if that was cleared too, update the top level
                block.map1 &= ~(1ull << o1);
            }
        }

        return n;
    }

    kassert(false, "frame_allocator ran out of free frames!");
    return 0;
}

void
frame_allocator::free(uintptr_t address, size_t count)
{
    util::scoped_lock lock {m_lock};

    kassert(address % frame_size == 0, "Trying to free a non page-aligned frame!");

    if (!count)
        return;

    for (long i = 0; i < m_count; ++i) {
        frame_block &block = m_blocks[i];
        uintptr_t end = block.base + block.count * frame_size;

        if (address < block.base || address >= end)
            continue;

        uint64_t frame = (address - block.base) >> 12;
        unsigned o1 = (frame >> 12) & 0x3f;
        unsigned o2 = (frame >> 6) & 0x3f;
        unsigned o3 = frame & 0x3f;

        while (count--) {
            block.map1 |= (1 << o1);
            block.map2[o1] |= (1 << o2);
            block.bitmap[o2] |= (1 << o3);
            if (++o3 == 64) {
                o3 = 0;
                if (++o2 == 64) {
                    o2 = 0;
                    ++o1;
                    kassert(o1 < 64, "Tried to free pages past the end of a block");
                }
            }
        }
    }
}

void
frame_allocator::used(uintptr_t address, size_t count)
{
    util::scoped_lock lock {m_lock};

    kassert(address % frame_size == 0, "Trying to mark a non page-aligned frame!");

    if (!count)
        return;

    for (long i = 0; i < m_count; ++i) {
        frame_block &block = m_blocks[i];
        uintptr_t end = block.base + block.count * frame_size;

        if (address < block.base || address >= end)
            continue;

        uint64_t frame = (address - block.base) >> 12;
        unsigned o1 = (frame >> 12) & 0x3f;
        unsigned o2 = (frame >> 6) & 0x3f;
        unsigned o3 = frame & 0x3f;

        while (count--) {
            block.bitmap[o2] &= ~(1 << o3);
            if (!block.bitmap[o2]) {
                block.map2[o1] &= ~(1 << o2);

                if (!block.map2[o1]) {
                    block.map1 &= ~(1 << o1);
                }
            }

            if (++o3 == 64) {
                o3 = 0;
                if (++o2 == 64) {
                    o2 = 0;
                    ++o1;
                    kassert(o1 < 64, "Tried to mark pages past the end of a block");
                }
            }
        }
    }
}

