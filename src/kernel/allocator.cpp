#include "kutil/memory_manager.h"
#include "log.h"

kutil::memory_manager g_kernel_memory_manager;

// kutil malloc/free implementation
namespace kutil {

void *
malloc(size_t n) {
	void *p = g_kernel_memory_manager.allocate(n);
	log::debug(logs::memory, "Heap allocated %ld bytes: %016lx", n, p);
	return p;
}

void free(void *p) { g_kernel_memory_manager.free(p); }

}
