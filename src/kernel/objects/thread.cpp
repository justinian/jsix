#include "j6/signals.h"
#include "objects/thread.h"
#include "objects/process.h"
#include "scheduler.h"

static constexpr j6_signal_t thread_default_signals = 0;

thread::thread(process &parent, uint8_t pri) :
	kobject(kobject::type::thread, thread_default_signals),
	m_parent(parent),
	m_state(state::loading),
	m_wait_type(wait_type::none),
	m_wait_data(0),
	m_wait_obj(0)
{
	TCB *tcbp = tcb();
	tcbp->pml4 = parent.pml4();
	tcbp->priority = pri;
	set_state(state::ready);
}

thread *
thread::from_tcb(TCB *tcb)
{
	static ptrdiff_t offset =
		-1 * static_cast<ptrdiff_t>(offsetof(thread, m_tcb));
	return reinterpret_cast<thread*>(kutil::offset_pointer(tcb, offset));
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
	m_return_code = code;
	set_state(state::exited);
	clear_state(state::ready);
}

