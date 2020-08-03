#include "kutil/vm_space.h"
#include "kernel_memory.h"
#include "page_manager.h"
#include "stack_cache.h"

extern kutil::vm_space g_kernel_space;

using memory::frame_size;
using memory::kernel_stack_pages;
static constexpr size_t stack_bytes = kernel_stack_pages * frame_size;

stack_cache stack_cache::s_instance(memory::stacks_start, memory::kernel_max_stacks);

stack_cache::stack_cache(uintptr_t start, size_t size) :
	m_next(start), m_end(start+size)
{
}

uintptr_t
stack_cache::get_stack()
{
	uintptr_t stack = 0;
	if (m_cache.count() > 0) {
		stack = m_cache.pop();
	} else {
		stack = m_next;
		m_next += stack_bytes;
	}

	g_kernel_space.commit(stack, stack_bytes);
	return stack;
}

void
stack_cache::return_stack(uintptr_t addr)
{
	void *ptr = reinterpret_cast<void*>(addr);
	page_manager::get()->unmap_pages(ptr, kernel_stack_pages);
	g_kernel_space.unreserve(addr, stack_bytes);
	m_cache.append(addr);
}
