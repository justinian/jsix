#pragma once
/// \file channel.hh
/// High level channel interface

// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <stddef.h>
#include <j6/types.h>
#include <util/api.h>

namespace util {
    class bip_buffer;
}

namespace j6 {

/// Descriptor of VMAs for a channel.
struct channel_def
{
    j6_handle_t tx;
    j6_handle_t rx;
};

struct channel_memory_area;

class API channel
{
public:
    /// Create a new channel.
    /// \arg tx_size  Size of the transmit buffer (sending from this thread)
    /// \arg rx_size  Size of the receive buffer (sending from remote thread),
    ///               or 0 for equal sized buffers.
    static channel * create(size_t tx_size, size_t rx_size = 0);

    /// Open an existing channel for which we have VMA handles
    static channel * open(const channel_def &def);

    /// Reserve an area of the output buffer for a write.
    /// \arg size  Requested size, in bytes
    /// \arg area  [out] Pointer to returned area
    /// \arg block If true, block this thread until there are size bytes free
    /// \returns   Size of returned area, in bytes, or 0 on failure
    size_t reserve(size_t size, uint8_t **area, bool block = true);

    /// Commit a pending write started by reserve()
    /// \arg size  Amount of data used, in bytes
    void commit(size_t size);

    /// Get a pointer to a block of data in the buffer.
    /// \arg area  [out] Pointer to the retuned area
    /// \arg block If true, block this thread until there is data available
    /// \returns   Size of the returned area, in bytes
    size_t get_block(uint8_t const **area, bool block = true) const;

    /// Mark a number of bytes as consumed, freeing buffer space
    /// \arg size  Number of bytes to consume
    void consume(size_t size);

    /// Get the channel_def for creating the remote end
    inline channel_def remote_def() const { return {m_def.rx, m_def.tx}; }

private:
    channel(
        const channel_def &def,
        channel_memory_area &tx,
        channel_memory_area &rx);

    channel_def m_def;
    channel_memory_area &m_tx;
    channel_memory_area &m_rx;
};

} // namespace j6

#endif // __j6kernel
