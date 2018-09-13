#pragma once
/// \file scheduler.h
/// The task scheduler and related definitions
#include "kutil/enum_bitfields.h"
#include "kutil/linked_list.h"
#include "kutil/memory.h"
#include "kutil/slab_allocator.h"
#include "kutil/vector.h"

class lapic;
struct page_table;
struct cpu_state;

extern "C" addr_t isr_handler(addr_t, cpu_state);

enum class process_flags : uint32_t
{
	running   = 0x00000001,
	ready     = 0x00000002,
	loading   = 0x00000004,

	const_pri = 0x80000000,

	none      = 0x00000000
};
IS_BITFIELD(process_flags);

/// A process
struct process
{
	uint32_t pid;
	uint32_t ppid;

	uint8_t priority;

	uint8_t reserved0;
	uint16_t reserved;

	process_flags flags;

	addr_t rsp;
	page_table *pml4;

	/// Helper to check if this process is ready
	/// \returns  true if the process has the ready flag
	inline bool ready() { return bitfield_has(flags, process_flags::ready); }
};

using process_list = kutil::linked_list<process>;
using process_node = process_list::item_type;
using process_slab = kutil::slab_allocator<process>;

/// The task scheduler
class scheduler
{
public:
	static const uint8_t num_priorities = 8;
	static const uint8_t default_priority = num_priorities / 2;

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

	/// Run the scheduler, possibly switching to a new task
	/// \arg rsp0  The stack pointer of the current interrupt handler
	/// \returns   The stack pointer to switch to
	addr_t schedule(addr_t rsp0);

	/// Get the current process.
	/// \returns  A pointer to the current process' process struct
	inline process * current() { return m_current; }

	/// Get a reference to the system scheduler
	/// \returns  A reference to the global system scheduler
	static scheduler & get() { return s_instance; }

	friend addr_t isr_handler(addr_t, cpu_state);

	/// Handle a timer tick
	/// \arg rsp0  The stack pointer of the current interrupt handler
	/// \returns   The stack pointer to switch to
	addr_t tick(addr_t rsp0);

private:
	lapic *m_apic;

	uint32_t m_next_pid;

	process_node *m_current;
	process_slab m_process_allocator;
	process_list m_runlists[num_priorities];
	process_list m_blocked;

	static scheduler s_instance;
};
