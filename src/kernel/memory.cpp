#include "assert.h"
#include "console.h"
#include "memory.h"
#include "memory_pages.h"

memory_manager *memory_manager::s_instance = nullptr;

memory_manager::memory_manager(page_block *free, page_block *used, void *scratch, size_t scratch_len)
{
}
