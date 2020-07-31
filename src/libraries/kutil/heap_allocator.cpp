#include <stdint.h>
#include "kutil/assert.h"
#include "kutil/memory.h"
#include "kutil/heap_allocator.h"

namespace kutil {

struct heap_allocator::mem_header
{
	mem_header(mem_header *prev, mem_header *next, uint8_t size) :
		m_prev(prev), m_next(next)
	{
		set_size(size);
	}

	inline void set_size(uint8_t size) {
		m_prev = reinterpret_cast<mem_header *>(
			reinterpret_cast<uintptr_t>(prev()) | (size & 0x3f));
	}

	inline void set_used(bool used) {
		m_next = reinterpret_cast<mem_header *>(
			reinterpret_cast<uintptr_t>(next()) | (used ? 1 : 0));
	}

	inline void set_next(mem_header *next) {
		bool u = used();
		m_next = next;
		set_used(u);
	}

	inline void set_prev(mem_header *prev) {
		uint8_t s = size();
		m_prev = prev;
		set_size(s);
	}

	void remove() {
		if (next()) next()->set_prev(prev());
		if (prev()) prev()->set_next(next());
		set_prev(nullptr);
		set_next(nullptr);
	}

	inline mem_header * next() { return kutil::mask_pointer(m_next, 0x3f); }
	inline mem_header * prev() { return kutil::mask_pointer(m_prev, 0x3f); }

	inline mem_header * buddy() const {
		return reinterpret_cast<mem_header *>(
			reinterpret_cast<uintptr_t>(this) ^ (1 << size()));
	}

	inline bool eldest() const { return this < buddy(); }

	inline uint8_t size() const { return reinterpret_cast<uintptr_t>(m_prev) & 0x3f; }
	inline bool used() const { return reinterpret_cast<uintptr_t>(m_next) & 0x1; }

private:
	mem_header *m_prev;
	mem_header *m_next;
};


heap_allocator::heap_allocator() : m_next(0), m_size(0) {}

heap_allocator::heap_allocator(uintptr_t start, size_t size) :
	m_next(start), m_size(size), m_allocated_size(0)
{
	kutil::memset(m_free, 0, sizeof(m_free));
}

void *
heap_allocator::allocate(size_t length)
{
	size_t total = length + sizeof(mem_header);
	unsigned size = min_size;
	while (total > (1 << size)) size++;

	kassert(size <= max_size, "Tried to allocate a block bigger than max_size");
	if (size > max_size)
		return nullptr;

	mem_header *header = pop_free(size);
	header->set_used(true);
	m_allocated_size += (1 << size);
	return header + 1;
}

void
heap_allocator::free(void *p)
{
	if (!p) return;

	mem_header *header = reinterpret_cast<mem_header *>(p);
	header -= 1; // p points after the header
	header->set_used(false);
	m_allocated_size -= (1 << header->size());

	while (header->size() != max_size) {
		auto size = header->size();

		mem_header *buddy = header->buddy();
		if (buddy->used() || buddy->size() != size)
			break;

		if (get_free(size) == buddy)
			get_free(size) = buddy->next();

		buddy->remove();

		header = header->eldest() ? header : buddy;
		header->set_size(size + 1);
	}

	uint8_t size = header->size();
	header->set_next(get_free(size));
	get_free(size) = header;
	if (header->next())
		header->next()->set_prev(header);
}

void
heap_allocator::ensure_block(unsigned size)
{
	if (get_free(size) != nullptr)
		return;

	if (size == max_size) {
		size_t bytes = (1 << max_size);
		if (bytes <= m_size) {
			mem_header *next = reinterpret_cast<mem_header *>(m_next);
			new (next) mem_header(nullptr, nullptr, size);
			get_free(size) = next;
			m_next += bytes;
			m_size -= bytes;
		}
	} else {
		mem_header *orig = pop_free(size + 1);
		if (orig) {
			mem_header *next = kutil::offset_pointer(orig, 1 << size);
			new (next) mem_header(orig, nullptr, size);

			orig->set_next(next);
			orig->set_size(size);
			get_free(size) = orig;
		}
	}
}

heap_allocator::mem_header *
heap_allocator::pop_free(unsigned size)
{
	ensure_block(size);
	mem_header *block = get_free(size);
	if (block) {
		get_free(size) = block->next();
		block->remove();
	}
	return block;
}

} // namespace kutil
