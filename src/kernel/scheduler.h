#pragma once
/// \file scheduler.h
/// The task scheduler and related definitions

#include <stdint.h>
#include "process.h"

class lapic;
struct page_table;
struct cpu_state;

extern "C" void isr_handler(cpu_state*);
extern "C" void task_switch(process *next);
extern "C" void task_fork(process *child);


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

	/// Loest (most urgent) priority achieved via promotion
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
	/// \arg pid      Pid to use for this task, must be negative
	/// \arg proc     Function to run as a kernel task
	/// \arg priority Priority to start the process with
	/// \arg flags    Flags to add to the process
	void create_kernel_task(
			pid_t pid,
			void (*task)(),
			uint8_t priority,
			process_flags flags = process_flags::none);

	/// Get the quantum for a given priority.
	uint32_t quantum(int priority);

	/// Start the scheduler working. This may involve starting
	/// timer interrupts or other preemption methods.
	void start();

	/// Run the scheduler, possibly switching to a new task
	void schedule();

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
	friend class process;

	/// Create a new process object. This process will have its pid
	/// set but nothing else.
	/// \arg pid  The pid to give the process (0 for automatic)
	/// \returns  The new process object
	process_node * create_process(pid_t pid = 0);

	void prune(uint64_t now);

	lapic *m_apic;

	uint32_t m_next_pid;
	uint32_t m_tick_count;

	process_node *m_current;
	process_list m_runlists[num_priorities];
	process_list m_blocked;
	process_list m_exited;

	// TODO: lol a real clock
	uint64_t m_clock = 0;

	static scheduler s_instance;
};
