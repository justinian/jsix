#pragma once
/// \file channel.hh
/// High level channel interface

// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <stddef.h>
#include <j6/types.h>

namespace j6 {

class channel
{
public:
    /// Create a new channel of the given size.
    static channel * create(size_t size);

    /// Open an existing channel for which we have a VMA handle
    static channel * open(j6_handle_t vma);

    /// Send data into the channel.
    /// \arg buffer  The buffer from which to read data
    /// \arg len     The number of bytes to read from `buffer`
    /// \arg block   If true, block this thread if there aren't `len` bytes of space available
    j6_status_t send(const void *buffer, size_t len, bool block = true);

    /// Read data out of the channel.
    /// \arg buffer  The buffer to receive channel data
    /// \arg size    [in] The size of `buffer` [out] the amount read into `buffer`
    /// \arg block   If true, block this thread if there is no data to read yet
    j6_status_t receive(void *buffer, size_t *size, bool block = true);

    /// Get the VMA handle for sharing with other processes
    j6_handle_t handle() const { return m_vma; }

private:
    struct header;

    channel(j6_handle_t vma, header *h);

    j6_handle_t m_vma;

    size_t m_size;
    header *m_header;
};

} // namespace j6

#endif // __j6kernel
