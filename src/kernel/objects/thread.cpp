#include "j6/signals.h"
#include "objects/thread.h"
#include "scheduler.h"

static constexpr j6_signal_t thread_default_signals = 0;

thread::thread(page_table *pml4, priority_t pri) :
	kobject(kobject::type::thread, thread_default_signals),
	m_state(state::loading),
	m_wait_type(wait_type::none),
	m_wait_data(0),
	m_wait_obj(0)
{
	TCB *tcbp = tcb();
	tcbp->pml4 = pml4;
	tcbp->priority = pri;
	tcbp->thread_data = this;
	set_state(state::ready);
}

void
thread::wait_on_signals(kobject *obj, j6_signal_t signals)
{
	m_wait_type = wait_type::signal;
	m_wait_data = signals;
	clear_state(state::ready);
}

void
thread::wait_on_time(uint64_t t)
{
	m_wait_type = wait_type::time;
	m_wait_data = t;
	clear_state(state::ready);
}

bool
thread::wake_on_signals(kobject *obj, j6_signal_t signals)
{
	if (m_wait_type != wait_type::signal ||
		(signals & m_wait_data) == 0)
		return false;

	m_wait_type = wait_type::none;
	m_wait_data = j6_status_ok;
	m_wait_obj = obj->koid();
	set_state(state::ready);
	return true;
}

bool
thread::wake_on_time(uint64_t now)
{
	if (m_wait_type != wait_type::time ||
		now < m_wait_data)
		return false;

	m_wait_type = wait_type::none;
	m_wait_data = j6_status_ok;
	m_wait_obj = 0;
	set_state(state::ready);
	return true;
}

void
thread::wake_on_result(kobject *obj, j6_status_t result)
{
	m_wait_type = wait_type::none;
	m_wait_data = result;
	m_wait_obj = obj->koid();
	set_state(state::ready);
}

void
thread::exit(uint32_t code)
{
	// TODO: check if the process containing this thread
	// needs to exit and clean up.
	m_return_code = code;
	set_state(state::exited);
	clear_state(state::ready);
}

