#include "process.h"


void
process::wait_on_signal(uint64_t sigmask)
{
	waiting = process_wait::signal;
	waiting_info = sigmask;
	flags -= process_flags::ready;
}

void
process::wait_on_child(uint32_t pid)
{
	waiting = process_wait::child;
	waiting_info = pid;
	flags -= process_flags::ready;
}

void
process::wait_on_time(uint64_t time)
{
	waiting = process_wait::time;
	waiting_info = time;
	flags -= process_flags::ready;
}

bool
process::wake_on_signal(int signal)
{
	if (waiting != process_wait::signal ||
		(waiting_info & (1 << signal)) == 0)
		return false;

	waiting = process_wait::none;
	flags += process_flags::ready;
	return true;
}

bool
process::wake_on_child(process *child)
{
	if (waiting != process_wait::child ||
		(waiting_info && waiting_info != child->pid))
		return false;

	waiting = process_wait::none;
	flags += process_flags::ready;
	return true;
}


bool
process::wake_on_time(uint64_t now)
{
	if (waiting != process_wait::time ||
		waiting_info > now)
		return false;

	waiting = process_wait::none;
	flags += process_flags::ready;
	return true;
}

