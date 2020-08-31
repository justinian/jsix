#include "j6/signals.h"
#include "log.h"
#include "objects/thread.h"
#include "objects/process.h"
#include "scheduler.h"
#include "buffer_cache.h"

extern "C" void kernel_to_user_trampoline();
static constexpr j6_signal_t thread_default_signals = 0;

thread::thread(process &parent, uint8_t pri, uintptr_t rsp0) :
	kobject(kobject::type::thread, thread_default_signals),
	m_parent(parent),
	m_state(state::loading),
	m_wait_type(wait_type::none),
	m_wait_data(0),
	m_wait_obj(0)
{
	m_tcb.pml4 = parent.pml4();
	m_tcb.priority = pri;

	if (!rsp0)
		setup_kernel_stack();
	else
		m_tcb.rsp0 = rsp0;

	set_state(state::ready);
}

thread::~thread()
{
	g_kstack_cache.return_buffer(m_tcb.kernel_stack);
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
	m_wait_result = j6_status_ok;
	m_wait_data = signals;
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
	m_wait_result = j6_status_ok;
	m_wait_data = now;
	m_wait_obj = 0;
	set_state(state::ready);
	return true;
}

void
thread::wake_on_result(kobject *obj, j6_status_t result)
{
	m_wait_type = wait_type::none;
	m_wait_result = result;
	m_wait_data = 0;
	m_wait_obj = obj->koid();
	set_state(state::ready);
}

void
thread::exit(uint32_t code)
{
	m_return_code = code;
	set_state(state::exited);
	clear_state(state::ready);
	assert_signal(j6_signal_thread_exit);
}

void
thread::add_thunk_kernel(uintptr_t rip)
{
	m_tcb.rsp -= sizeof(uintptr_t) * 7;
	uintptr_t *stack = reinterpret_cast<uintptr_t*>(m_tcb.rsp);

	stack[6] = rip;        // return rip
	stack[5] = m_tcb.rsp0; // rbp
	stack[4] = 0xbbbbbbbb; // rbx
	stack[3] = 0x12121212; // r12
	stack[2] = 0x13131313; // r13
	stack[1] = 0x14141414; // r14
	stack[0] = 0x15151515; // r15
}

void
thread::add_thunk_user(uintptr_t rip)
{
	m_tcb.rsp -= sizeof(uintptr_t) * 8;
	uintptr_t *stack = reinterpret_cast<uintptr_t*>(m_tcb.rsp);

	stack[7] = rip;        // return rip in rcx
	stack[6] = m_tcb.rsp3; // rbp
	stack[5] = 0xbbbbbbbb; // rbx
	stack[4] = 0x00000200; // r11 sets RFLAGS
	stack[3] = 0x12121212; // r12
	stack[2] = 0x13131313; // r13
	stack[1] = 0x14141414; // r14
	stack[0] = 0x15151515; // r15

	static const uintptr_t trampoline =
		reinterpret_cast<uintptr_t>(kernel_to_user_trampoline);
	add_thunk_kernel(trampoline);
}

void
thread::setup_kernel_stack()
{
	using memory::frame_size;
	using memory::kernel_stack_pages;
	static constexpr size_t stack_bytes = kernel_stack_pages * frame_size;

	constexpr unsigned null_frame_entries = 2;
	constexpr size_t null_frame_size = null_frame_entries * sizeof(uint64_t);

	uintptr_t stack_addr = g_kstack_cache.get_buffer();
	uintptr_t stack_end = stack_addr + stack_bytes;

	uint64_t *null_frame = reinterpret_cast<uint64_t*>(stack_end - null_frame_size);
	for (unsigned i = 0; i < null_frame_entries; ++i)
		null_frame[i] = 0;

	log::debug(logs::memory, "Created kernel stack at %016lx size 0x%lx",
			stack_addr, stack_bytes);

	m_tcb.kernel_stack = stack_addr;
	m_tcb.rsp0 = reinterpret_cast<uintptr_t>(null_frame);
	m_tcb.rsp = m_tcb.rsp0;
}

thread *
thread::create_idle_thread(process &kernel, uint8_t pri, uintptr_t rsp0)
{
	thread *idle = new thread(kernel, pri, rsp0);
	idle->set_state(thread::state::constant);
	log::info(logs::task, "Created idle thread as koid %llx", idle->koid());

	return idle;
}
