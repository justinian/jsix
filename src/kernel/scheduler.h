#pragma once
/// \file scheduler.h
/// The task scheduler and related definitions
#include "kutil/memory.h"
#include "kutil/vector.h"

class lapic;
struct page_table;


struct process
{
	uint16_t pid;
	addr_t rsp;
	page_table *pml4;
};


/// The task scheduler
class scheduler
{
public:
	/// Constructor.
	/// \arg apic  Pointer to the local APIC object
	scheduler(lapic *apic);

	/// Create a new process from a program image in memory.
	/// \arg name  Name of the program image
	/// \arg data  Pointer to the image data
	/// \arg size  Size of the program image, in bytes
	void create_process(const char *name, const void *data, size_t size);

	/// Start the scheduler working. This may involve starting
	/// timer interrupts or other preemption methods.
	void start();

	/// Handle a timer tick
	/// \arg rsp0  The stack pointer of the current interrupt handler
	/// \returns   The stack pointer to handler to switch to
	addr_t tick(addr_t rsp0);

	/// Get a reference to the system scheduler
	/// \returns  A reference to the global system scheduler
	static scheduler & get() { return s_instance; }

private:
	lapic *m_apic;
	kutil::vector<process> m_processes;
	uint16_t m_current;
	uint16_t m_next_pid;

	static scheduler s_instance;
};
