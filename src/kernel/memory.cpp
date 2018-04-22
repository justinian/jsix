#include "assert.h"
#include "console.h"
#include "memory.h"
#include "memory_pages.h"

memory_manager g_memory_manager;

memory_manager::memory_manager()
{
	kassert(this == &g_memory_manager, "Attempt to create another memory_manager.");
}
