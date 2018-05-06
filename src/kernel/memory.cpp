#include "kutil/memory.h"
#include "assert.h"
#include "log.h"
#include "memory.h"
#include "memory_pages.h"

memory_manager g_kernel_memory_manager;

struct memory_manager::alloc_header
{
	uint64_t size;
	uint64_t reserved;
	uint8_t data[0];
} __attribute__ ((packed));

struct memory_manager::free_header
{
	free_header *next;
	uint64_t size;
} __attribute__ ((packed));

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
	size_t total = length + sizeof(alloc_header);
	unsigned size = min_size;
	while (total > (1 << size)) size++;
	kassert(size < max_size, "Tried to allocate a block bigger than max_size");
	log::debug(logs::memory, "Allocating %d bytes, which is size %d", total, size);

	alloc_header *header = reinterpret_cast<alloc_header *>(pop_free(size));
	header->size = size;

	log::debug(logs::memory, "  Returning %d bytes at %lx", length, &header->data);
	return &header->data;
}

void
memory_manager::free(void *p)
{
	// In this simple version, we don't care about freed pointers
}

void
memory_manager::grow_memory()
{
	size_t length = (1 << max_size);

	free_header *block = reinterpret_cast<free_header *>(
			reinterpret_cast<uint64_t>(m_start) + m_length);

	g_page_manager.map_pages(
			reinterpret_cast<page_manager::addr_t>(block),
			length / page_manager::page_size);

	block->size = max_size;
	block->next = get_free(max_size);
	get_free(max_size) = block;
	m_length += length;

	log::debug(logs::memory, "Allocated new block at %lx: size %d next %lx",
			block, block->size, block->next);
}

void
memory_manager::ensure_block(unsigned size)
{
	free_header *header = get_free(size);
	if (header != nullptr) return;
	else if (size == max_size) {
		grow_memory();
		return;
	}

	free_header *bigger = pop_free(size + 1);

	uint64_t new_size = bigger->size - 1;
	free_header *next = reinterpret_cast<free_header *>(
			reinterpret_cast<uint64_t>(bigger) + (1 << new_size));

	next->next = bigger->next;
	next->size = new_size;

	bigger->next = next;
	bigger->size = new_size;
	log::debug(logs::memory, "ensure_block[%2d] split blocks:", size);
	log::debug(logs::memory, "   %lx: size %d next %lx", bigger, bigger->size, bigger->next);
	log::debug(logs::memory, "   %lx: size %d next %lx", next, next->size, next->next);

	get_free(size) = bigger;
}

memory_manager::free_header *
memory_manager::pop_free(unsigned size)
{
	ensure_block(size);
	free_header *block = get_free(size);
	get_free(size) = block->next;
	block->next = nullptr;
	return block;
}

void * operator new (size_t, void *p) { return p; }
void * operator new (size_t n)    { return g_kernel_memory_manager.allocate(n); }
void * operator new[] (size_t n)  { return g_kernel_memory_manager.allocate(n); }
void operator delete (void *p)  { return g_kernel_memory_manager.free(p); }
void operator delete[] (void *p){ return g_kernel_memory_manager.free(p); }
