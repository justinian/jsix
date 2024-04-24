#pragma once
/// \file bip_buffer.h
/// A Bip Buffer (bipartite circular buffer). For more on the Bip Buffer structure, see
/// https://www.codeproject.com/Articles/3479/The-Bip-Buffer-The-Circular-Buffer-with-a-Twist

#include <stddef.h>
#include <stdint.h>

#include <util/api.h>
#include <util/spinlock.h>

namespace util {

class API bip_buffer
{
public:
    /// Constructor.
    bip_buffer(size_t size);

    /// Reserve an area of buffer for a write.
    /// \arg size  Requested size, in bytes
    /// \arg area  [out] Pointer to returned area
    /// \returns   Size of returned area, in bytes, or 0 on failure
    size_t reserve(size_t size, uint8_t **area);

    /// Commit a pending write started by reserve()
    /// \arg size  Amount of data used, in bytes
    void commit(size_t size);

    /// Get a pointer to a block of data in the buffer.
    /// \arg area  [out] Pointer to the retuned area
    /// \returns   Size of the returned area, in bytes
    size_t get_block(uint8_t const **area) const;

    /// Mark a number of bytes as consumed, freeing buffer space
    /// \arg size  Number of bytes to consume
    void consume(size_t size);

    /// Get total amount of data in the buffer.
    /// \returns  Number of bytes committed to the buffer
    inline size_t size() const { return m_size_a + m_size_b; }

    /// Get total amount of free buffer remaining
    /// \returns  Number of bytes of buffer that are free
    inline size_t free_space() const { return m_buffer_size - size(); }

    /// Get total size of the buffer
    /// \returns  Number of bytes in the buffer
    inline size_t buffer_size() const { return m_buffer_size; }

    /// Get the current size available for a contiguous write
    size_t write_available() const;

private:
    size_t m_start_a;
    size_t m_start_b;
    size_t m_start_r;
    size_t m_size_a;
    size_t m_size_b;
    size_t m_size_r;

    mutable spinlock m_lock;
    const size_t m_buffer_size;
    uint8_t m_buffer[0];

    bip_buffer() = delete;
};

} // namespace util
