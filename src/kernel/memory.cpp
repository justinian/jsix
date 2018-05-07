#include "kutil/enum_bitfields.h"
#include "kutil/memory.h"
#include "assert.h"
#include "log.h"
#include "memory.h"
#include "memory_pages.h"

memory_manager g_kernel_memory_manager;

struct memory_manager::mem_header
{
	mem_header(mem_header *prev, mem_header *next, uint8_t size) :
		m_prev(prev), m_next(next)
	{
		set_size(size);
	}

	inline void set_size(uint8_t size)
	{
		m_prev = reinterpret_cast<mem_header *>(
			reinterpret_cast<addr_t>(prev()) | (size & 0x3f));
	}

	inline void set_used(bool used)
	{
		m_next = reinterpret_cast<mem_header *>(
			reinterpret_cast<addr_t>(next()) | (used ? 1 : 0));
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
			reinterpret_cast<addr_t>(this) ^ (1 << size()));
	}

	inline bool eldest() const { return this < buddy(); }

	inline uint8_t size() const { return reinterpret_cast<addr_t>(m_prev) & 0x3f; }
	inline bool used() const { return reinterpret_cast<addr_t>(m_next) & 0x1; }

private:
	mem_header *m_prev;
	mem_header *m_next;
};


memory_manager::memory_manager() :
	m_start(nullptr),
	m_length(0)
{
	kutil::memset(m_free, 0, sizeof(m_free));
}

memory_manager::memory_manager(void *start) :
	m_start(start),
	m_length(0)
{
	kutil::memset(m_free, 0, sizeof(m_free));
	grow_memory();
}

void *
memory_manager::allocate(size_t length)
{
	size_t total = length + sizeof(mem_header);
	unsigned size = min_size;
	while (total > (1 << size)) size++;
	kassert(size < max_size, "Tried to allocate a block bigger than max_size");
	log::debug(logs::memory, "Allocating %d bytes, which is size %d", total, size);

	mem_header *header = pop_free(size);
	header->set_used(true);

	log::debug(logs::memory, "  Returning %d bytes at %lx", length, header + 1);
	return header + 1;
}

void
memory_manager::free(void *p)
{
	mem_header *header = reinterpret_cast<mem_header *>(p);
	header -= 1; // p points after the header
	header->set_used(false);

	log::debug(logs::memory, "Freeing a block of size %2d at %lx", header->size(), header);

	while (true) {
		mem_header *buddy = header->buddy();
		if (buddy->used() || buddy->size() != header->size()) break;
		log::debug(logs::memory, "  buddy is same size at %lx", buddy);
		buddy->remove();
		header = header->eldest() ? header : buddy;
		header->set_size(header->size() + 1);
		log::debug(logs::memory, "  joined into size %2d at %lx", header->size(), header);
	}

	uint8_t size = header->size();
	header->set_next(get_free(size));
	get_free(size) = header;
	if (header->next())
		header->next()->set_prev(header);
}

void
memory_manager::grow_memory()
{
	size_t length = (1 << max_size);

	void *next = kutil::offset_pointer(m_start, m_length);

	g_page_manager.map_pages(
			reinterpret_cast<page_manager::addr_t>(next),
			length / page_manager::page_size);

	mem_header *block = new (next) mem_header(nullptr, get_free(max_size), max_size);
	get_free(max_size) = block;
	if (block->next())
		block->next()->set_prev(block);
	m_length += length;

	log::debug(logs::memory, "Allocated new block at %lx: size %d next %lx",
			block, max_size, block->next());
}

void
memory_manager::ensure_block(unsigned size)
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

	log::debug(logs::memory, "ensure_block[%2d] split blocks:", size);
	log::debug(logs::memory, "   %lx: size %d next %lx", orig, size, orig->next());
	log::debug(logs::memory, "   %lx: size %d next %lx", next, size, next->next());
}

memory_manager::mem_header *
memory_manager::pop_free(unsigned size)
{
	ensure_block(size);
	mem_header *block = get_free(size);
	get_free(size) = block->next();

	block->remove();
	return block;
}

void * operator new (size_t, void *p) { return p; }
void * operator new (size_t n)    { return g_kernel_memory_manager.allocate(n); }
void * operator new[] (size_t n)  { return g_kernel_memory_manager.allocate(n); }
void operator delete (void *p)  { return g_kernel_memory_manager.free(p); }
void operator delete[] (void *p){ return g_kernel_memory_manager.free(p); }
