#include "kutil/assert.h"
#include "kutil/frame_allocator.h"
#include "kutil/memory.h"

namespace kutil {

using memory::frame_size;
using memory::page_offset;

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
	m_cache.append(cache);
}

void
frame_allocator::init(
	frame_block_list free,
	frame_block_list used)
{
	m_free.append(free);
	m_used.append(used);
}

list_node<frame_block> *
frame_allocator::get_block_node()
{
	if (m_cache.empty()) {
		auto *first = m_free.front();

		frame_block_node * start =
			reinterpret_cast<frame_block_node*>(first->address + page_offset);
		frame_block_node * end = offset_pointer(start, frame_size);

		if (first->count == 1) {
			m_free.remove(first);
		} else {
			first->count--;
			first->address += frame_size;
		}

		while (start < end) {
			m_cache.push_back(start);
			start++;
		}
	}

	return m_cache.pop_front();
}

void
frame_allocator::consolidate_blocks()
{
	m_cache.append(frame_block::consolidate(m_free));
	m_cache.append(frame_block::consolidate(m_used));
}

size_t
frame_allocator::allocate(size_t count, uintptr_t *address)
{
	kassert(!m_free.empty(), "frame_allocator::pop_frames ran out of free frames!");

	auto *first = m_free.front();

	unsigned n = count < first->count ? count : first->count;
	*address = first->address;

	if (count >= first->count) {
		m_free.remove(first);
		m_used.sorted_insert(first);
	} else {
		auto *used = get_block_node();
		used->copy(first);
		used->count = n;
		m_used.sorted_insert(used);

		first->address += n * frame_size;
		first->count -= n;
	}

	consolidate_blocks();
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

			auto *lead_block = get_block_node();

			lead_block->copy(block);
			lead_block->count = frames;

			block->count -= frames;
			block->address += leading;

			m_used.insert_before(block, lead_block);
		}

		if (trailing) {
			size_t frames = trailing / frame_size;

			auto *trail_block = get_block_node();

			trail_block->copy(block);
			trail_block->count = frames;
			trail_block->address += size;
			block->count -= frames;

			m_used.insert_before(block, trail_block);
		}

		m_used.remove(block);
		m_free.sorted_insert(block);
		++block_count;

		address += block->count * frame_size;
		count -= block->count;
		if (!count)
			break;
	}

	kassert(block_count, "Couldn't find existing allocated frames to free");
	consolidate_blocks();
}

} // namespace kutil
