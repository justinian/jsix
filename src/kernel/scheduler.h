#pragma once
/// \file scheduler.h
/// The task scheduler and related definitions

#include <stdint.h>
#include "kutil/spinlock.h"
#include "kutil/vector.h"

namespace kernel {
namespace args {
	struct program;
}}

struct cpu_data;
class lapic;
class process;
struct page_table;
struct run_queue;


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

	/// How many quanta a process gets before being rescheduled
	static const uint16_t process_quanta = 10;

	/// Constructor.
	/// \arg cpus  The number of CPUs to schedule for
	scheduler(unsigned cpus);
	~scheduler();

	/// Create a new process from a program image in memory.
	/// \arg program  The descriptor of the pogram in memory
	/// \returns      The main thread of the loaded process
	thread * load_process(kernel::args::program &program);

	/// Create a new kernel task
	/// \arg proc     Function to run as a kernel task
	/// \arg priority Priority to start the process with
	/// \arg constant True if this task cannot be promoted/demoted
	void create_kernel_task(
			void (*task)(),
			uint8_t priority,
			bool constant = false);

	/// Get the quantum for a given priority.
	static uint32_t quantum(int priority);

	/// Start the scheduler working. This may involve starting
	/// timer interrupts or other preemption methods.
	void start();

	/// Run the scheduler, possibly switching to a new task
	void schedule();

	/// Start scheduling a new thread.
	/// \arg t  The new thread's TCB
	void add_thread(TCB *t);

	/// Get a reference to the scheduler
	/// \returns  A reference to the global system scheduler
	static scheduler & get() { return *s_instance; }

private:
	friend class process;

	static constexpr uint64_t promote_frequency = 10;
	static constexpr uint64_t steal_frequency = 10;

	void prune(run_queue &queue, uint64_t now);
	void check_promotions(run_queue &queue, uint64_t now);
	void steal_work(cpu_data &cpu);

	uint32_t m_next_pid;
	uint32_t m_tick_count;

	process *m_kernel_process;

	kutil::vector<run_queue> m_run_queues;

	// TODO: lol a real clock
	uint64_t m_clock = 0;

	unsigned m_steal_turn = 0;
	static scheduler *s_instance;
};

