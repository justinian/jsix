#include <new>
#include <stdint.h>
#include <string.h>

#include <util/pointers.h>
#include <util/util.h>

#include "assert.h"
#include "heap_allocator.h"
#include "memory.h"

uint32_t & get_map_key(heap_allocator::block_info &info) { return info.offset; }

struct heap_allocator::free_header
{
    void clear(unsigned new_order, free_header *new_next = nullptr) {
        prev = nullptr;
        next = new_next;
        order = new_order;
    }

    void remove() {
        if (next) next->prev = prev;
        if (prev) prev->next = next;
        prev = next = nullptr;
    }

    inline free_header * buddy() const {
        return reinterpret_cast<free_header *>(
            reinterpret_cast<uintptr_t>(this) ^ (1ull << order));
    }

    inline bool eldest() const { return this < buddy(); }

    free_header *prev;
    free_header *next;
    unsigned order;
};


heap_allocator::heap_allocator() : m_start {0}, m_end {0} {}

heap_allocator::heap_allocator(uintptr_t start, size_t size, uintptr_t heapmap) :
    m_start {start},
    m_end {start},
    m_maxsize {size},
    m_allocated_size {0},
    m_map (reinterpret_cast<block_info*>(heapmap), 512)

{
    memset(m_free, 0, sizeof(m_free));
}

void *
heap_allocator::allocate(size_t length)
{
    if (length == 0)
        return nullptr;

    unsigned order = util::log2(length);
    if (order < min_order)
        order = min_order;

    kassert(order <= max_order, "Tried to allocate a block bigger than max_order");
    if (order > max_order)
        return nullptr;

    util::scoped_lock lock {m_lock};

    m_allocated_size += (1ull << order);

    free_header *block = pop_free(order);
    if (!block && !split_off(order, block)) {
        return new_block(order);
    }

    m_map[map_key(block)].free = false;

    return block;
}

void
heap_allocator::free(void *p)
{
    if (!p) return;

    uintptr_t addr = reinterpret_cast<uintptr_t>(p);
    kassert(addr >= m_start && addr < m_end,
        "Attempt to free non-heap pointer");

    util::scoped_lock lock {m_lock};

    free_header *block = reinterpret_cast<free_header *>(p);
    block_info *info = m_map.find(map_key(block));
    kassert(info, "Attempt to free pointer not known to the heap");
    if (!info) return;

    size_t size = (1ull << info->order);
    m_allocated_size -= size;

    block->clear(info->order);
    block = merge_block(block);
    register_free_block(block, block->order);
}

void *
heap_allocator::reallocate(void *p, size_t old_length, size_t new_length)
{
    if (!p) {
        kassert(old_length == 0,
            "Attempt to reallocate from null with non-zero old_length");
        return allocate(new_length);
    }

    util::scoped_lock lock {m_lock};

    block_info *info = m_map.find(map_key(p));
    kassert(info, "Attempt to reallocate unknown block");
    if (!info)
        return nullptr;

    if (new_length <= (1ull << info->order))
        return p;

    lock.release();
    void *new_block = allocate(new_length);
    memcpy(new_block, p, old_length);
    free(p);

    return new_block;
}

heap_allocator::free_header *
heap_allocator::pop_free(unsigned order)
{
    free_header *& head = get_free(order);
    free_header *block = head;
    if (block) {
        kassert(block->prev == nullptr, "freelist head had non-null prev");
        head = block->next;
        block->remove();
    }
    return block;
}

heap_allocator::free_header *
heap_allocator::merge_block(free_header *block)
{
    // The lock needs to be held while calling merge_block

    unsigned order = block->order;
    while (order < max_order) {
        free_header *buddy = block->buddy();

        block_info *info = m_map.find(map_key(buddy));
        if (!info || !info->free || info->order != order)
            break;

        free_header *&head = get_free(order);
        if (head == buddy)
            pop_free(order);
        else
            buddy->remove();

        block = block->eldest() ? block : buddy;

        m_map.erase(map_key(block->buddy()));
        block->order = m_map[map_key(block)].order = ++order;
    }

    return block;
}

void *
heap_allocator::new_block(unsigned order)
{
    // The lock needs to be held while calling new_block

    // Add the largest blocks possible until m_end is
    // aligned to be a block of the requested order
    unsigned current = address_order(m_end);
    while (current < order) {
        register_free_block(reinterpret_cast<free_header*>(m_end), current);
        m_end += 1ull << current;
        current = address_order(m_end);
    }

    void *block = reinterpret_cast<void*>(m_end);
    m_end += 1ull << order;
    m_map[map_key(block)].order = order;
    return block;
}

void
heap_allocator::register_free_block(free_header *block, unsigned order)
{
    // The lock needs to be held while calling register_free_block

    block_info &info = m_map[map_key(block)];
    info.free = true;
    info.order = order;

    free_header *& head = get_free(order);
    if (head)
        head->prev = block;

    block->clear(order, head);
    head = block;
}

bool
heap_allocator::split_off(unsigned order, free_header *&split)
{
    // The lock needs to be held while calling split_off

    const unsigned next = order + 1;
    if (next > max_order)
        return false;

    free_header *block = pop_free(next);
    if (!block && !split_off(next, block))
        return false;

    block->order = order;
    register_free_block(block->buddy(), order);
    m_map[map_key(block)].order = order;

    split = block;
    return true;
}
