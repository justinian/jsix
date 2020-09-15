#pragma once
/// \file frame_allocator.h
/// Allocator for physical memory frames

#include <stdint.h>

#include "kutil/linked_list.h"

struct frame_block;
using frame_block_list = kutil::linked_list<frame_block>;

/// Allocator for physical memory frames
class frame_allocator
{
public:
	/// Default constructor
	frame_allocator();

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

private:
	frame_block_list m_free; ///< Free frames list

	frame_allocator(const frame_allocator &) = delete;
};


/// A block of contiguous frames. Each `frame_block` represents contiguous
/// physical frames with the same attributes.
struct frame_block
{
	uintptr_t address;
	uint32_t count;

	/// Compare two blocks by address.
	/// \arg rhs   The right-hand comparator
	/// \returns   <0 if this is sorts earlier, >0 if this sorts later, 0 for equal
	int compare(const frame_block &rhs) const;
};

