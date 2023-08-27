#pragma once
/// \file allocator.h
/// Definition of the allocator interface.

#include <stddef.h>
#include <j6/memutils.h>

namespace util {

// Allocators are types with three static methods with these signatures
struct default_allocator
{
    inline static void * allocate(size_t size) { return new char[size]; }
    inline static void free(void *p) { delete [] reinterpret_cast<char*>(p); }

    inline static void * realloc(void *p, size_t oldsize, size_t newsize) {
        void *newp = memcpy(allocate(newsize), p, oldsize);
        free(p);
        return newp;
    }
};

struct null_allocator
{
    inline static void * allocate(size_t size) { return nullptr; }
    inline static void free(void *p) {}
    inline static void * realloc(void *p, size_t, size_t) { return p; }
};


} // namespace util
