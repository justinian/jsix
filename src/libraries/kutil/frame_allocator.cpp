#include <algorithm>

#include "kutil/frame_allocator.h"

namespace kutil {

int
frame_block::compare(const frame_block *rhs) const
{
	if (address < rhs->address)
		return -1;
	else if (address > rhs->address)
		return 1;

	return 0;
}

frame_block_list
frame_block::consolidate(frame_block_list &list)
{
	frame_block_list freed;

	for (auto *cur : list) {
		auto *next = cur->next();

		while ( next &&
				cur->flags == next->flags &&
				cur->end() == next->address) {
			cur->count += next->count;
			list.remove(next);
			freed.push_back(next);
		}
	}

	return freed;
}

void
frame_block::zero()
{
	address = 0;
	count = 0;
	flags = frame_block_flags::none;
}

void
frame_block::copy(frame_block *other)
{
	address = other->address;
	count = other->count;
	flags = other->flags;
}


frame_allocator::frame_allocator(
	frame_block_list cache)
{
	m_block_slab.append(cache);
}

void
frame_allocator::init(
	frame_block_list free,
	frame_block_list used)
{
	m_free.append(free);
	m_used.append(used);
	consolidate_blocks();
}

void
frame_allocator::consolidate_blocks()
{
	m_block_slab.append(frame_block::consolidate(m_free));
	m_block_slab.append(frame_block::consolidate(m_used));
}

size_t
frame_allocator::allocate(size_t count, uintptr_t *address)
{
	kassert(!m_free.empty(), "frame_allocator::pop_frames ran out of free frames!");

	auto *first = m_free.front();

	unsigned n = std::min(count, static_cast<size_t>(first->count));
	*address = first->address;

	if (count >= first->count) {
		m_free.remove(first);
		m_used.sorted_insert(first);
	} else {
		auto *used = m_block_slab.pop();
		used->copy(first);
		used->count = n;
		m_used.sorted_insert(used);

		first->address += n * frame_size;
		first->count -= n;
	}

	m_block_slab.append(frame_block::consolidate(m_used));
	return n;
}

void
frame_allocator::free(uintptr_t address, size_t count)
{
	size_t block_count = 0;

	for (auto *block : m_used) {
		if (!block->contains(address)) continue;

		size_t size = frame_size * count;
		uintptr_t end = address + size;

		size_t leading = address - block->address;
		size_t trailing =
			end > block->end() ?
			0 : (block->end() - end);

		if (leading) {
			size_t frames = leading / frame_size;

			auto *lead_block = m_block_slab.pop();

			lead_block->copy(block);
			lead_block->count = frames;

			block->count -= frames;
			block->address += leading;

			m_used.insert_before(block, lead_block);
		}

		if (trailing) {
			size_t frames = trailing / frame_size;

			auto *trail_block = m_block_slab.pop();

			trail_block->copy(block);
			trail_block->count = frames;
			trail_block->address += size;
			trail_block->address += size;

			block->count -= frames;

			m_used.insert_after(block, trail_block);
		}

		address += block->count * frame_size;

		m_used.remove(block);
		m_free.sorted_insert(block);
		++block_count;
	}

	kassert(block_count, "Couldn't find existing allocated frames to free");
}

} // namespace kutil
