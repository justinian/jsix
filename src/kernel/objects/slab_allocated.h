#pragma once
/// \file slab_allocated.h
/// A parent template class for slab-allocated objects

#include <stdlib.h>
#include <util/pointers.h>
#include <util/vector.h>

#include "memory.h"

extern uintptr_t g_slabs_bump_pointer;

template <typename T, unsigned N = 1>
class slab_allocated
{
public:
    constexpr static unsigned slab_pages = N;
    constexpr static unsigned slab_size = N * mem::frame_size;

    void * operator new(size_t size)
    {
        kassert(size == sizeof(T), "Slab allocator got wrong size allocation");
        if (s_free.count() == 0)
            allocate_chunk();

        T *item = s_free.pop();
        memset(item, 0, sizeof(T));
        return item;
    }

    void operator delete(void *p) { s_free.append(reinterpret_cast<T*>(p)); }

private:
    constexpr static size_t per_block = slab_size / sizeof(T);

    static void allocate_chunk()
    {
        s_free.ensure_capacity(per_block);

        uintptr_t start = __atomic_fetch_add(
                &g_slabs_bump_pointer, slab_size,
                __ATOMIC_SEQ_CST);

        T *current = reinterpret_cast<T*>(start);
        T *end = util::offset_pointer(current, slab_size);
        while (current < end)
            s_free.append(current++);
    }

    static util::vector<T*> s_free;
};

template <typename T, unsigned N>
util::vector<T*> slab_allocated<T,N>::s_free;

#define DEFINE_SLAB_ALLOCATOR(type) \
    template<> util::vector<typename type*> slab_allocated<typename type, type::slab_pages>::s_free {};
