#include "kutil/assert.h"
#include "buffer_cache.h"
#include "kernel_memory.h"
#include "objects/vm_area.h"
#include "page_manager.h"
#include "vm_space.h"

extern vm_space g_kernel_space;

using memory::frame_size;
using memory::kernel_stack_pages;
using memory::kernel_buffer_pages;

static constexpr size_t stack_bytes = kernel_stack_pages * frame_size;
static constexpr size_t buffer_bytes = kernel_buffer_pages * frame_size;

buffer_cache g_kstack_cache {memory::stacks_start, stack_bytes, memory::kernel_max_stacks};
buffer_cache g_kbuffer_cache {memory::buffers_start, buffer_bytes, memory::kernel_max_buffers};

buffer_cache::buffer_cache(uintptr_t start, size_t size, size_t length) :
	m_next(start), m_end(start+length), m_size(size)
{
	kassert((size % frame_size) == 0, "buffer_cache given a non-page-multiple size!");
}

uintptr_t
buffer_cache::get_buffer()
{
	uintptr_t addr = 0;
	if (m_cache.count() > 0) {
		addr = m_cache.pop();
	} else {
		addr = m_next;
		m_next += m_size;
	}

	vm_space &vm = vm_space::kernel_space();
	vm.allow(addr, m_size, true);

	return addr;
}

void
buffer_cache::return_buffer(uintptr_t addr)
{
	vm_space &vm = vm_space::kernel_space();
	vm.allow(addr, m_size, false);

	m_cache.append(addr);
}
