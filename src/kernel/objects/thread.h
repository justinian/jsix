#pragma once
/// \file thread.h
/// Definition of thread kobject types

#include "kutil/linked_list.h"
#include "objects/kobject.h"

struct page_table;
class process;

struct TCB
{
	// Data used by assembly task control routines.  If you change any of these,
	// be sure to change the assembly definitions in 'tasking.inc'
	uintptr_t rsp;
	uintptr_t rsp0;
	uintptr_t rsp3;
	page_table *pml4;

	uint8_t priority;
	// note: 3 bytes padding

	// TODO: move state into TCB?

	uintptr_t kernel_stack;
	size_t kernel_stack_size;

	uint32_t time_left;
	uint64_t last_ran;

	thread *thread_data;
};

using tcb_list = kutil::linked_list<TCB>;
using tcb_node = tcb_list::item_type;

class thread :
	public kobject
{
public:
	enum class wait_type : uint8_t { none, signal, time };
	enum class state : uint8_t {
		ready    = 0x01,
		loading  = 0x02,
		exited   = 0x04,
		constant = 0x80,
		none     = 0x00
	};

	/// Get the `ready` state of the thread.
	/// \returns True if the thread is ready to execute.
	inline bool ready() const { return has_state(state::ready); }

	/// Get the `loading` state of the thread.
	/// \returns True if the thread has not finished loading.
	inline bool loading() const { return has_state(state::loading); }

	/// Get the `constant` state of the thread.
	/// \returns True if the thread has constant priority.
	inline bool constant() const { return has_state(state::constant); }

	/// Get the thread priority.
	inline uint8_t priority() const { return m_tcb.priority; }

	/// Set the thread priority.
	/// \arg p  The new thread priority
	inline void set_priority(uint8_t p) { if (!constant()) m_tcb.priority = p; }

	/// Block the thread, waiting on the given object's signals.
	/// \arg obj     Object to wait on
	/// \arg signals Mask of signals to wait for
	void wait_on_signals(kobject *obj, j6_signal_t signals);

	/// Block the thread, waiting for a given clock value
	/// \arg t  Clock value to wait for
	void wait_on_time(uint64_t t);

	/// Wake the thread if it is waiting on signals.
	/// \arg obj     Object that changed signals
	/// \arg signals Signal state of the object
	/// \returns     True if this action unblocked the thread
	bool wake_on_signals(kobject *obj, j6_signal_t signals);

	/// Wake the thread if it is waiting on the clock.
	/// \arg now  Current clock value
	/// \returns  True if this action unblocked the thread
	bool wake_on_time(uint64_t now);

	/// Wake the thread with a given result code.
	/// \arg obj     Object that changed signals
	/// \arg result  Result code to return to the thread
	void wake_on_result(kobject *obj, j6_status_t result);

	inline bool has_state(state s) const {
		return static_cast<uint8_t>(m_state) & static_cast<uint8_t>(s);
	}

	inline void set_state(state s) {
		m_state = static_cast<state>(static_cast<uint8_t>(m_state) | static_cast<uint8_t>(s));
	}

	inline void clear_state(state s) {
		m_state = static_cast<state>(static_cast<uint8_t>(m_state) & ~static_cast<uint8_t>(s));
	}

	inline tcb_node * tcb() { return &m_tcb; }
	inline process & parent() { return m_parent; }

	/// Terminate this thread.
	/// \arg code   The return code to exit with.
	void exit(unsigned code);

private:
	thread() = delete;
	thread(const thread &other) = delete;
	thread(const thread &&other) = delete;
	friend class process;

	/// Constructor.
	/// \arg p   The process which owns this thread
	/// \arg pri Initial priority level of this thread
	thread(process &parent, uint8_t pri);

	process &m_parent;

	tcb_node m_tcb;

	state m_state;
	wait_type m_wait_type;
	// There should be 1 byte of padding here

	uint32_t m_return_code;

	uint64_t m_wait_data;
	j6_koid_t m_wait_obj;
};
