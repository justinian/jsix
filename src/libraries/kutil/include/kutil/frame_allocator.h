#pragma once
/// \file frame_allocator.h
/// Allocator for physical memory frames

#include <stdint.h>

#include "kutil/enum_bitfields.h"
#include "kutil/linked_list.h"
#include "kutil/slab_allocator.h"

namespace kutil {

struct frame_block;
using frame_block_list = kutil::linked_list<frame_block>;
using frame_block_slab = kutil::slab_allocator<frame_block>;

/// Allocator for physical memory frames
class frame_allocator
{
public:
	/// Size of a single page frame.
	static const size_t frame_size = 0x1000;

	/// Constructor. Sets up the frame allocator from bootstraped memory.
	/// \arg free   List of free blocks
	/// \arg used   List of currently used blocks
	/// \arg cache  List of pre-allocated but unused frame_block structures
	frame_allocator(
			frame_block_list free,
			frame_block_list used,
			frame_block_list cache);

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
	/// Consolidate the free and used block lists. Return freed blocks
	/// to the cache.
	void consolidate_blocks();

	frame_block_list m_free; ///< Free frames list
	frame_block_list m_used; ///< In-use frames list
	frame_block_slab m_block_slab; ///< frame_block slab allocator

	frame_allocator(const frame_allocator &) = delete;
};


/// Flags used by `frame_block`.
enum class frame_block_flags : uint32_t
{
	none         = 0x00,

	mmio         = 0x01,  ///< Memory is a MMIO region
	nonvolatile  = 0x02,  ///< Memory is non-volatile storage

	pending_free = 0x10,  ///< Memory should be freed
	acpi_wait    = 0x40,  ///< Memory should be freed after ACPI init
	permanent    = 0x80   ///< Memory is permanently unusable
};

} // namespace kutil

IS_BITFIELD(kutil::frame_block_flags);

namespace kutil {

/// A block of contiguous frames. Each `frame_block` represents contiguous
/// physical frames with the same attributes.
struct frame_block
{
	uintptr_t address;
	uint32_t count;
	frame_block_flags flags;

	inline bool has_flag(frame_block_flags f) const { return bitfield_has(flags, f); }
	inline uintptr_t end() const { return address + (count * frame_allocator::frame_size); }
	inline bool contains(uintptr_t addr) const { return addr >= address && addr < end(); }

	/// Helper to zero out a block and optionally set the next pointer.
	void zero();

	/// Helper to copy a bock from another block
	/// \arg other  The block to copy from
	void copy(frame_block *other);

	/// Compare two blocks by address.
	/// \arg rhs   The right-hand comparator
	/// \returns   <0 if this is sorts earlier, >0 if this sorts later, 0 for equal
	int compare(const frame_block *rhs) const;

	/// Traverse the list, joining adjacent blocks where possible.
	/// \arg list  The list to consolidate
	/// \returns   A linked list of freed frame_block structures.
	static frame_block_list consolidate(frame_block_list &list);
};

} // namespace kutil

