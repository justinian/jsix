#include "cpu.h"
#include "log.h"
#include "process.h"
#include "scheduler.h"


pid_t
process::fork(uintptr_t in_rsp)
{
	auto &sched = scheduler::get();
	auto *child = sched.create_process();
	kassert(child, "process::fork() got null child");

	child->ppid = pid;
	child->flags =
		process_flags::running |
		process_flags::ready;

	sched.m_runlists[child->priority].push_back(child);

	child->rsp = in_rsp;

	child->pml4 = page_manager::get()->copy_table(pml4);
	kassert(child->pml4, "process::fork() got null pml4");

	child->setup_kernel_stack(kernel_stack_size, kernel_stack);
	child->rsp = child->kernel_stack + (in_rsp - kernel_stack);

	log::debug(logs::task, "Copied process %d to %d, new PML4 %016lx.",
			pid, child->pid, child->pml4);
	log::debug(logs::task, "  copied stack %016lx to %016lx, rsp %016lx to %016lx.",
			kernel_stack, child->kernel_stack, in_rsp, child->rsp);

	// Add in the faked fork return value
	cpu_state *regs = reinterpret_cast<cpu_state *>(child->rsp);
	regs->rax = 0;

	return child->pid;
}

void *
process::setup_kernel_stack(size_t size, uintptr_t orig)
{
	void *stack0 = kutil::malloc(size);

	if (orig)
		kutil::memcpy(stack0, reinterpret_cast<void*>(orig), size);
	else
		kutil::memset(stack0, 0, size);

	kernel_stack_size = size;
	kernel_stack = reinterpret_cast<uintptr_t>(stack0);

	return stack0;
}

bool
process::wait_on_signal(uint64_t sigmask)
{
	waiting = process_wait::signal;
	waiting_info = sigmask;
	flags -= process_flags::ready;
	return true;
}

bool
process::wait_on_child(uint32_t pid)
{
	waiting = process_wait::child;
	waiting_info = pid;
	flags -= process_flags::ready;
	return true;
}

bool
process::wait_on_time(uint64_t time)
{
	waiting = process_wait::time;
	waiting_info = time;
	flags -= process_flags::ready;
	return true;
}

bool
process::wait_on_send(uint32_t target_id)
{
	scheduler &s = scheduler::get();
	process *target = s.get_process_by_id(target_id);
	if (!target) return false;

	if (!target->wake_on_receive(this)) {
		waiting = process_wait::send;
		waiting_info = target_id;
		flags -= process_flags::ready;
	}

	return true;
}

bool
process::wait_on_receive(uint32_t source_id)
{
	scheduler &s = scheduler::get();
	process *source = s.get_process_by_id(source_id);
	if (!source) return false;

	if (!source->wake_on_send(this)) {
		waiting = process_wait::receive;
		waiting_info = source_id;
		flags -= process_flags::ready;
		return true;
	}

	return false;
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

bool
process::wake_on_send(process *target)
{
	if (waiting != process_wait::send ||
		waiting_info != target->pid)
		return false;

	waiting = process_wait::none;
	flags += process_flags::ready;
	return true;
}


bool
process::wake_on_receive(process *source)
{
	if (waiting != process_wait::receive ||
		waiting_info != source->pid)
		return false;

	waiting = process_wait::none;
	flags += process_flags::ready;
	return true;
}

