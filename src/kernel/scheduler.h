#pragma once
/// \file scheduler.h
/// The task scheduler and related definitions

#include <stdint.h>
#include "objects/thread.h"

class lapic;
struct page_table;
struct cpu_state;

extern "C" void isr_handler(cpu_state*);
extern "C" void task_switch(TCB *next);


/// The task scheduler
class scheduler
{
public:
	/// Total number of priority levels
	static const uint8_t num_priorities = 8;

	/// Maximum (least urgent/interactive) priority
	static const uint8_t max_priority = num_priorities - 1;

	/// Default priority on process creation
	static const uint8_t default_priority = 1;

	/// Lowest (most urgent) priority achieved via promotion
	static const uint8_t promote_limit = 1;

	/// How long the base timer quantum is, in us
	static const uint64_t quantum_micros = 500;

	/// How much extra time (in us) to give a process to let it load
	static const uint64_t startup_bonus = 16000;

	/// How many quanta a process gets before being rescheduled
	static const uint16_t process_quanta = 10;

	/// Constructor.
	/// \arg apic  Pointer to the local APIC object
	scheduler(lapic *apic);

	/// Create a new process from a program image in memory.
	/// \arg name  Name of the program image
	/// \arg data  Pointer to the image data
	/// \arg size  Size of the program image, in bytes
	void load_process(const char *name, const void *data, size_t size);

	/// Create a new kernel task
	/// \arg proc     Function to run as a kernel task
	/// \arg priority Priority to start the process with
	/// \arg constant True if this task cannot be promoted/demoted
	void create_kernel_task(
			void (*task)(),
			uint8_t priority,
			bool constant = false);

	/// Get the quantum for a given priority.
	uint32_t quantum(int priority);

	/// Start the scheduler working. This may involve starting
	/// timer interrupts or other preemption methods.
	void start();

	/// Run the scheduler, possibly switching to a new task
	void schedule();

	/// Get the current TCB.
	/// \returns  A pointer to the current thread's TCB
	inline TCB * current() { return m_current; }

	/*
	/// Look up a process by its PID
	/// \arg pid  The requested PID
	/// \returns  The process matching that PID, or nullptr
	tcb_node * get_process_by_id(uint32_t pid);
	*/

	/// Get a reference to the system scheduler
	/// \returns  A reference to the global system scheduler
	static scheduler & get() { return s_instance; }

private:
	friend uintptr_t syscall_dispatch(uintptr_t, cpu_state &);
	friend class process;

	/// Create a new process object. This process will have its pid
	/// set but nothing else.
	/// \arg pml4 The root page table of the process
	/// \returns  The new process' main thread
	thread * create_process(page_table *pml4);

	void prune(uint64_t now);

	lapic *m_apic;

	uint32_t m_next_pid;
	uint32_t m_tick_count;

	tcb_node *m_current;
	tcb_list m_runlists[num_priorities];
	tcb_list m_blocked;
	tcb_list m_exited;

	// TODO: lol a real clock
	uint64_t m_clock = 0;

	static scheduler s_instance;
};
