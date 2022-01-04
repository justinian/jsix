#pragma once
/// \file frame_allocator.h
/// Allocator for physical memory frames

#include <stdint.h>
#include <util/spinlock.h>

namespace bootproto {
    struct frame_block;
}

/// Allocator for physical memory frames
class frame_allocator
{
public:
    using frame_block = bootproto::frame_block;

    /// Constructor
    /// \arg blocks The bootloader-supplied frame bitmap block list
    /// \arg count  Number of entries in the block list
    frame_allocator(frame_block *frames, size_t count);

    /// Get free frames from the free list. Only frames from the first free block
    /// are returned, so the number may be less than requested, but they will
    /// be contiguous.
    /// \arg count    The maximum number of frames to get
    /// \arg address  [out] The physical address of the first frame
    /// \returns      The number of frames retrieved
    size_t allocate(size_t count, uintptr_t *address);

    /// Free previously allocated frames.
    /// \arg address  The physical address of the first frame to free
    /// \arg count    The number of frames to be freed
    void free(uintptr_t address, size_t count);

    /// Mark frames as used
    /// \arg address  The physical address of the first frame to free
    /// \arg count    The number of frames to be freed
    void used(uintptr_t address, size_t count);

    /// Get the global frame allocator
    static frame_allocator & get();

private:
    frame_block *m_blocks;
    size_t m_count;

    util::spinlock m_lock;

    frame_allocator() = delete;
    frame_allocator(const frame_allocator &) = delete;
};
