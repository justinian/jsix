#include <stdint.h>
#include <string.h>
#include "kutil/util.h"

#include "assert.h"
#include "heap_allocator.h"
#include "memory.h"

struct heap_allocator::mem_header
{
    mem_header(mem_header *prev, mem_header *next, uint8_t order) :
        m_prev(prev), m_next(next)
    {
        set_order(order);
    }

    inline void set_order(uint8_t order) {
        m_prev = reinterpret_cast<mem_header *>(
            reinterpret_cast<uintptr_t>(prev()) | (order & 0x3f));
    }

    inline void set_used(bool used) {
        m_next = reinterpret_cast<mem_header *>(
            reinterpret_cast<uintptr_t>(next()) | (used ? 1 : 0));
    }

    inline void set_next(mem_header *next) {
        bool u = used();
        m_next = next;
        set_used(u);
    }

    inline void set_prev(mem_header *prev) {
        uint8_t s = order();
        m_prev = prev;
        set_order(s);
    }

    void remove() {
        if (next()) next()->set_prev(prev());
        if (prev()) prev()->set_next(next());
        set_prev(nullptr);
        set_next(nullptr);
    }

    inline mem_header * next() { return mask_pointer(m_next, 0x3f); }
    inline mem_header * prev() { return mask_pointer(m_prev, 0x3f); }

    inline mem_header * buddy() const {
        return reinterpret_cast<mem_header *>(
            reinterpret_cast<uintptr_t>(this) ^ (1 << order()));
    }

    inline bool eldest() const { return this < buddy(); }

    inline uint8_t order() const { return reinterpret_cast<uintptr_t>(m_prev) & 0x3f; }
    inline bool used() const { return reinterpret_cast<uintptr_t>(m_next) & 0x1; }

private:
    mem_header *m_prev;
    mem_header *m_next;
};


heap_allocator::heap_allocator() : m_start {0}, m_end {0} {}

heap_allocator::heap_allocator(uintptr_t start, size_t size) :
    m_start {start},
    m_end {start+size},
    m_blocks {0},
    m_allocated_size {0}
{
    memset(m_free, 0, sizeof(m_free));
}

void *
heap_allocator::allocate(size_t length)
{
    size_t total = length + sizeof(mem_header);

    if (length == 0)
        return nullptr;

    unsigned order = kutil::log2(total);
    if (order < min_order)
        order = min_order;

    kassert(order <= max_order, "Tried to allocate a block bigger than max_order");
    if (order > max_order)
        return nullptr;

    kutil::scoped_lock lock {m_lock};

    mem_header *header = pop_free(order);
    header->set_used(true);
    m_allocated_size += (1 << order);
    return header + 1;
}

void
heap_allocator::free(void *p)
{
    if (!p) return;

    uintptr_t addr = reinterpret_cast<uintptr_t>(p);
    kassert(addr >= m_start && addr < m_end,
        "Attempt to free non-heap pointer");

    kutil::scoped_lock lock {m_lock};

    mem_header *header = reinterpret_cast<mem_header *>(p);
    header -= 1; // p points after the header
    header->set_used(false);
    m_allocated_size -= (1 << header->order());

    while (header->order() != max_order) {
        auto order = header->order();

        mem_header *buddy = header->buddy();
        if (buddy->used() || buddy->order() != order)
            break;

        if (get_free(order) == buddy)
            get_free(order) = buddy->next();

        buddy->remove();

        header = header->eldest() ? header : buddy;
        header->set_order(order + 1);
    }

    uint8_t order = header->order();
    header->set_next(get_free(order));
    get_free(order) = header;
    if (header->next())
        header->next()->set_prev(header);
}

void
heap_allocator::ensure_block(unsigned order)
{
    if (get_free(order) != nullptr)
        return;

    if (order == max_order) {
        size_t bytes = (1 << max_order);
        uintptr_t next = m_start + m_blocks * bytes;
        if (next + bytes <= m_end) {
            mem_header *nextp = reinterpret_cast<mem_header *>(next);
            new (nextp) mem_header(nullptr, nullptr, order);
            get_free(order) = nextp;
            ++m_blocks;
        }
    } else {
        mem_header *orig = pop_free(order + 1);
        if (orig) {
            mem_header *next = offset_pointer(orig, 1 << order);
            new (next) mem_header(orig, nullptr, order);

            orig->set_next(next);
            orig->set_order(order);
            get_free(order) = orig;
        }
    }
}

heap_allocator::mem_header *
heap_allocator::pop_free(unsigned order)
{
    ensure_block(order);
    mem_header *block = get_free(order);
    if (block) {
        get_free(order) = block->next();
        block->remove();
    }
    return block;
}
