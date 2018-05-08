#include "kutil/memory_manager.h"

kutil::memory_manager g_kernel_memory_manager;

void * kalloc(size_t length) { return g_kernel_memory_manager.allocate(length); }
void kfree(void *p) { g_kernel_memory_manager.free(p); }
void * operator new (size_t n)    { return g_kernel_memory_manager.allocate(n); }
void * operator new[] (size_t n)  { return g_kernel_memory_manager.allocate(n); }
void operator delete (void *p)  { return g_kernel_memory_manager.free(p); }
void operator delete[] (void *p){ return g_kernel_memory_manager.free(p); }
