#include "j6/signals.h"
#include "kutil/assert.h"
#include "cpu.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "page_manager.h"

kutil::vector<process*> process::s_processes;

process::process(page_table *pml4) :
	kobject(kobject::type::process),
	m_pml4(pml4),
	m_space(pml4),
	m_next_handle(0),
	m_state(state::running)
{
	s_processes.append(this);
}

process::~process()
{
	s_processes.remove_swap(this);
}

process &
process::current()
{
	return *bsp_cpu_data.p;
}

void
process::exit(unsigned code)
{
	// TODO: make this thread-safe
	if (m_state != state::running)
		return;
	else
		m_state = state::exited;

	for (auto *thread : m_threads) {
		thread->exit(code);
	}
	m_return_code = code;
	page_manager::get()->delete_process_map(m_pml4);
	assert_signal(j6_signal_process_exit);
}

void
process::update()
{
	kassert(m_threads.count() > 0, "process::update with zero threads!");

	size_t i = 0;
	uint32_t status = 0;
	while (i < m_threads.count()) {
		thread *th = m_threads[i];
		if (th->has_state(thread::state::exited)) {
			status = th->m_return_code;
			m_threads.remove_swap_at(i);
			continue;
		}
		i++;
	}

	if (m_threads.count() == 0) {
		// TODO: What really is the return code in this case?
		exit(status);
	}
}

thread *
process::create_thread(uint8_t priority, bool user)
{
	thread *th = new thread(*this, priority);
	kassert(th, "Failed to create thread!");

	if (user) {
		uintptr_t stack_top = stacks_top - (m_threads.count() * stack_size);
		auto *pm = page_manager::get();
		pm->map_pages(
			stack_top - stack_size,
			page_manager::page_count(stack_size),
			true, // user stack
			m_pml4);

		th->tcb()->rsp3 = stack_top;
	}

	m_threads.append(th);
	return th;
}

bool
process::thread_exited(thread *th)
{
	kassert(&th->m_parent == this, "Process got thread_exited for non-child!");
	uint32_t status = th->m_return_code;
	m_threads.remove_swap(th);
	delete th;

	if (m_threads.count() == 0) {
		exit(status);
		return true;
	}

	return false;
}

j6_handle_t
process::add_handle(kobject *obj)
{
	if (!obj)
		return j6_handle_invalid;

	obj->handle_retain();
	j6_handle_t handle = m_next_handle++;
	m_handles.insert(handle, obj);
	return handle;
}

bool
process::remove_handle(j6_handle_t handle)
{
	kobject *obj = m_handles.find(handle);
	if (obj) obj->handle_release();
	return m_handles.erase(handle);
}

kobject *
process::lookup_handle(j6_handle_t handle)
{
	return m_handles.find(handle);
}
