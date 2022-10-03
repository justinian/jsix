#pragma once
/// \file heap_allocator.h
/// A buddy allocator for a memory heap

#include <stddef.h>

#include <util/spinlock.h>
#include <util/node_map.h>

/// Allocator for a given heap range
class heap_allocator
{
public:
    /// Default constructor creates a valid but empty heap.
    heap_allocator();

    /// Constructor. The given memory area must already have been reserved.
    /// \arg start   Starting address of the heap
    /// \arg size    Size of the heap in bytes
    /// \arg heapmap Starting address of the heap tracking map
    heap_allocator(uintptr_t start, size_t size, uintptr_t heapmap);

    /// Allocate memory from the area managed.
    /// \arg length  The amount of memory to allocate, in bytes
    /// \returns     A pointer to the allocated memory, or nullptr if
    ///              allocation failed.
    void * allocate(size_t length);

    /// Free a previous allocation.
    /// \arg p  A pointer previously retuned by allocate()
    void free(void *p);

    /// Minimum block size is (2^min_order). Must be at least 6.
    static const unsigned min_order = 6;  // 2^6  == 64 B

    /// Maximum block size is (2^max_order). Must be less than 32 + min_order.
    static const unsigned max_order = 22; // 2^22 ==  4 MiB

protected:
    struct free_header;
    struct block_info
    {
        uint32_t offset;
        uint8_t order;
        bool free;
    };
    friend uint32_t & get_map_key(block_info &info);

    inline uint32_t map_key(void *p) const {
        return static_cast<uint32_t>(
               (reinterpret_cast<uintptr_t>(p) - m_start) >> min_order);
    }

    using block_map = util::inplace_map<uint32_t, block_info, -1u>;

    /// Get the largest block size order that aligns with this address
    inline unsigned address_order(uintptr_t addr) {
        unsigned tz = __builtin_ctzll(addr);
        return tz > max_order ? max_order : tz;
    }

    /// Helper accessor for the list of blocks of a given order
    /// \arg order  Order (2^N) of the block we want
    /// \returns    A mutable reference to the head of the list
    free_header *& get_free(unsigned order)  { return m_free[order - min_order]; }

    /// Helper to remove and return the first block in the free
    /// list for the given order.
    free_header * pop_free(unsigned order);

    /// Merge the given block with any currently free buddies to
    /// create the largest block possible.
    /// \arg block  The current block
    /// \returns    The fully-merged block
    free_header * merge_block(free_header *block);

    /// Create a new block of the given order past the end of the existing
    /// heap. The block will be marked as non-free.
    /// \arg order  The requested size order
    /// \returns    A pointer to the block's memory
    void * new_block(unsigned order);

    /// Register the given block as free with the given order.
    /// \arg block  The newly-created or freed block
    /// \arg order  The size order to set on the block
    void register_free_block(free_header *block, unsigned order);

    /// Helper to get a block of the given order by splitting existing
    /// larger blocks. Returns false if there were no larger blocks.
    /// \arg order  Order (2^N) of the block we want
    /// \arg block  [out] Receives a pointer to the requested block
    /// \returns    True if a split was done
    bool split_off(unsigned order, free_header *&block);

    uintptr_t m_start, m_end;
    size_t m_maxsize;
    free_header *m_free[max_order - min_order + 1];
    size_t m_allocated_size;

    util::spinlock m_lock;
    block_map m_map;

    heap_allocator(const heap_allocator &) = delete;
};
