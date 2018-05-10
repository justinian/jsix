#include "kutil/memory_manager.h"

kutil::memory_manager g_kernel_memory_manager;

// kutil malloc/free implementation
namespace kutil {
	void * malloc(size_t n) { return g_kernel_memory_manager.allocate(n); }
	void free(void *p) { g_kernel_memory_manager.free(p); }
}
