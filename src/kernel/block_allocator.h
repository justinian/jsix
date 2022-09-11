#pragma once
/// \file block_allocator.h
/// Definitions and functions for a generic block allocator

#include <stddef.h>
#include <util/vector.h>

class block_allocator
{
public:
    inline block_allocator(uintptr_t start, size_t size) :
        m_next_block(start), m_block_size(size) {}

    inline uintptr_t allocate() {
        if (m_free.count() > 0)
            return m_free.pop();
        else
            return __atomic_fetch_add(&m_next_block, m_block_size, __ATOMIC_SEQ_CST);
    }

    inline void free(uintptr_t p) { m_free.append(p); }
    inline uintptr_t end() const { return m_next_block; }

private:
    uintptr_t m_next_block;
    const size_t m_block_size;
    util::vector<uintptr_t> m_free;
};
