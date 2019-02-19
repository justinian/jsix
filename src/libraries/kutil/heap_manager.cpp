#include <stdint.h>
#include "kutil/assert.h"
#include "kutil/memory.h"
#include "kutil/heap_manager.h"

namespace kutil {


struct heap_manager::mem_header
{
	mem_header(mem_header *prev, mem_header *next, uint8_t size) :
		m_prev(prev), m_next(next)
	{
		set_size(size);
	}

	inline void set_size(uint8_t size)
	{
		m_prev = reinterpret_cast<mem_header *>(
			reinterpret_cast<uintptr_t>(prev()) | (size & 0x3f));
	}

	inline void set_used(bool used)
	{
		m_next = reinterpret_cast<mem_header *>(
			reinterpret_cast<uintptr_t>(next()) | (used ? 1 : 0));
	}

	inline void set_next(mem_header *next)
	{
		bool u = used();
		m_next = next;
		set_used(u);
	}

	inline void set_prev(mem_header *prev)
	{
		uint8_t s = size();
		m_prev = prev;
		set_size(s);
	}

	void remove()
	{
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


heap_manager::heap_manager() :
	m_start(nullptr),
	m_length(0),
	m_grow(nullptr)
{
	kutil::memset(m_free, 0, sizeof(m_free));
}

heap_manager::heap_manager(void *start, grow_callback grow_cb) :
	m_start(start),
	m_length(0),
	m_grow(grow_cb)
{
	kutil::memset(m_free, 0, sizeof(m_free));
	grow_memory();
}

void *
heap_manager::allocate(size_t length)
{
	size_t total = length + sizeof(mem_header);
	unsigned size = min_size;
	while (total > (1 << size)) size++;
	kassert(size <= max_size, "Tried to allocate a block bigger than max_size");

	mem_header *header = pop_free(size);
	header->set_used(true);
	return header + 1;
}

void
heap_manager::free(void *p)
{
	mem_header *header = reinterpret_cast<mem_header *>(p);
	header -= 1; // p points after the header
	header->set_used(false);

	while (header->size() != max_size) {
		mem_header *buddy = header->buddy();
		if (buddy->used() || buddy->size() != header->size()) break;
		buddy->remove();
		header = header->eldest() ? header : buddy;
		header->set_size(header->size() + 1);
	}

	uint8_t size = header->size();
	header->set_next(get_free(size));
	get_free(size) = header;
	if (header->next())
		header->next()->set_prev(header);
}

void
heap_manager::grow_memory()
{
	size_t length = (1 << max_size);

	kassert(m_grow, "Tried to grow heap without a growth callback");
	void *next = m_grow(kutil::offset_pointer(m_start, m_length), length);

	mem_header *block = new (next) mem_header(nullptr, get_free(max_size), max_size);
	get_free(max_size) = block;
	if (block->next())
		block->next()->set_prev(block);
	m_length += length;
}

void
heap_manager::ensure_block(unsigned size)
{
	if (get_free(size) != nullptr) return;
	else if (size == max_size) {
		grow_memory();
		return;
	}

	mem_header *orig = pop_free(size + 1);

	mem_header *next = kutil::offset_pointer(orig, 1 << size);
	new (next) mem_header(orig, nullptr, size);

	orig->set_next(next);
	orig->set_size(size);
	get_free(size) = orig;
}

heap_manager::mem_header *
heap_manager::pop_free(unsigned size)
{
	ensure_block(size);
	mem_header *block = get_free(size);
	get_free(size) = block->next();

	block->remove();
	return block;
}

} // namespace kutil
