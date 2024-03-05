#pragma once
/// \file j6/ring_buffer.hh
/// Helper class for managing ring buffers in doubly-mapped VMAs

// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel
#include <stddef.h>
#include <j6/types.h>
#include <util/api.h>

namespace j6 {

API class ring_buffer
{
public:
    ring_buffer(size_t pages);

    inline bool valid() const { return m_data != nullptr; }
    inline size_t size() const { return 1<<m_bits; }
    inline size_t used() const { return m_write-m_read; }
    inline size_t free() const { return size()-used(); }

    inline const char * read_ptr() const { return get(m_read); }
    inline char * write_ptr() { return get(m_write); }

    inline void commit(size_t &n) {
        if (n > free()) n = free();
        m_write += n;
    }

    inline const char * consume(size_t &n) {
        if (n > used()) n = used();
        size_t i = m_read;
        m_read += n;
        return get(i);
    }

private:
    inline size_t mask() const { return (1<<m_bits)-1;}
    inline size_t index(size_t i) const { return i & mask(); }

    inline char * get(size_t i) { return m_data + index(i); }
    inline const char * get(size_t i) const { return m_data + index(i); }

    size_t m_bits;
    size_t m_write;
    size_t m_read;
    j6_handle_t m_vma;
    char *m_data;
};

} // namespace j6
#endif // __j6kernel

