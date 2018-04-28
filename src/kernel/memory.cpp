#include "assert.h"
#include "console.h"
#include "memory.h"
#include "memory_pages.h"

memory_manager g_kernel_memory_manager;

struct memory_allocation_header
{
	uint64_t pages;
	uint64_t reserved;
	uint8_t data[0];
} __attribute__ ((packed));

memory_manager::memory_manager() :
	m_start(nullptr),
	m_length(0)
{
}

memory_manager::memory_manager(void *start, size_t length) :
	m_start(start),
	m_length(length)
{
}

void *
memory_manager::allocate(size_t length)
{
	length = page_align(length + sizeof(memory_allocation_header));

	if (length > m_length) return nullptr;

	m_length -= length;
	memory_allocation_header *header =
		reinterpret_cast<memory_allocation_header *>(m_start);
	m_start = reinterpret_cast<uint8_t *>(m_start) + length;

	size_t pages = length / page_manager::page_size;
	g_page_manager.map_pages(
			reinterpret_cast<page_manager::addr_t>(header),
			pages);

	header->pages = pages;
	return &header->data;
}

void
memory_manager::free(void *p)
{
	// In this simple version, we don't care about freed pointers
}
