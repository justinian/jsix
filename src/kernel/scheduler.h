#pragma once
/// \file scheduler.h
/// The task scheduler and related definitions

#include <stdint.h>
#include "kutil/slab_allocator.h"
#include "process.h"

class lapic;
struct page_table;
struct cpu_state;

extern "C" uintptr_t isr_handler(uintptr_t, cpu_state*);


/// The task scheduler
class scheduler
{
public:
	static const uint8_t num_priorities = 8;
	static const uint8_t default_priority = num_priorities / 2;

	/// How long the timer quantum is
	static const uint64_t quantum_micros = 100000;

	/// How many quantums a process gets before being rescheduled
	static const uint16_t process_quanta = 10;

	/// Constructor.
	/// \arg apic  Pointer to the local APIC object
	scheduler(lapic *apic);

	/// Create a new process from a program image in memory.
	/// \arg name  Name of the program image
	/// \arg data  Pointer to the image data
	/// \arg size  Size of the program image, in bytes
	void load_process(const char *name, const void *data, size_t size);

	/// Start the scheduler working. This may involve starting
	/// timer interrupts or other preemption methods.
	void start();

	/// Run the scheduler, possibly switching to a new task
	/// \arg rsp0  The stack pointer of the current interrupt handler
	/// \returns   The stack pointer to switch to
	uintptr_t schedule(uintptr_t rsp0);

	/// Get the current process.
	/// \returns  A pointer to the current process' process struct
	inline process * current() { return m_current; }

	/// Look up a process by its PID
	/// \arg pid  The requested PID
	/// \returns  The process matching that PID, or nullptr
	process_node * get_process_by_id(uint32_t pid);

	/// Get a reference to the system scheduler
	/// \returns  A reference to the global system scheduler
	static scheduler & get() { return s_instance; }

private:
	friend uintptr_t syscall_dispatch(uintptr_t, cpu_state &);
	friend uintptr_t isr_handler(uintptr_t, cpu_state*);
	friend class process;

	/// Create a new process object. This process will have its pid
	/// set but nothing else.
	/// \returns  The new process object
	process_node * create_process();

	/// Handle a timer tick
	/// \arg rsp0  The stack pointer of the current interrupt handler
	/// \returns   The stack pointer to switch to
	uintptr_t tick(uintptr_t rsp0);

	void prune(uint64_t now);

	lapic *m_apic;

	uint32_t m_next_pid;
	uint32_t m_tick_count;

	using process_slab = kutil::slab_allocator<process>;
	process_slab m_process_allocator;

	process_node *m_current;
	process_list m_runlists[num_priorities];
	process_list m_blocked;
	process_list m_exited;

	static scheduler s_instance;
};
