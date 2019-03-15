#pragma once
/// \file process.h
/// The processes and related definitions

#include <stdint.h>
#include "kutil/enum_bitfields.h"
#include "kutil/linked_list.h"
#include "page_manager.h"

typedef uint32_t pid_t;


enum class process_flags : uint32_t
{
	running   = 0x00000001,
	ready     = 0x00000002,
	loading   = 0x00000004,

	const_pri = 0x80000000,

	none      = 0x00000000
};
IS_BITFIELD(process_flags);

enum class process_wait : uint8_t
{
	none,
	signal,
	child,
	time,
	send,
	receive
};

/// A process
struct process
{
	pid_t pid;
	pid_t ppid;

	process_flags flags;

	uint16_t quanta;

	uint8_t priority;

	process_wait waiting;
	uint64_t waiting_info;

	uint32_t return_code;

	uint32_t reserved1;

	uintptr_t rsp;
	page_table *pml4;

	uintptr_t kernel_stack;
	size_t kernel_stack_size;

	/// Terminate this process.
	/// \arg code   The return code to exit with.
	void exit(unsigned code);

	/// Copy this process.
	/// \arg in_rsp The RSP of the calling process
	/// \returns    Returns the child's pid to the parent, and
	///             0 to the child.
	pid_t fork(uint64_t in_rsp);

	/// Unready this process until it gets a signal
	/// \arg sigmask  A bitfield of signals to wake on
	/// \returns  Whether the process should be rescheduled
	bool wait_on_signal(uint64_t sigmask);

	/// Unready this process until a child exits
	/// \arg pid  PID of the child to wait for, or 0 for any
	/// \returns  Whether the process should be rescheduled
	bool wait_on_child(uint32_t pid);

	/// Unready this process until after the given time
	/// \arg time  The time after which to wake
	/// \returns  Whether the process should be rescheduled
	bool wait_on_time(uint64_t time);

	/// Try to send to the target process, becoming unready if it
	/// is not waiting on receive.
	/// \arg target_id  The process to send to
	/// \returns  Whether the process should be rescheduled
	bool wait_on_send(uint32_t target_id);

	/// Try to receive from one or more processes, becoming unready
	/// if none of them are waiting on a send to this process.
	/// \arg source_id  The process to receive from
	/// \returns  Whether the process should be rescheduled
	bool wait_on_receive(uint32_t source_id);

	/// If this process is waiting on the given signal, wake it
	/// \argument signal  The signal sent to the process
	/// \returns  True if this wake was handled
	bool wake_on_signal(int signal);

	/// If this process is waiting on the given child, wake it
	/// \argument child  The process that exited
	/// \returns  True if this wake was handled
	bool wake_on_child(process *child);

	/// If this process is waiting on a time, check it
	/// \argument now  The current time
	/// \returns  True if this wake was handled
	bool wake_on_time(uint64_t now);

	/// If this process is waiting to send to this target, wake it
	/// \argument target  The target process
	/// \returns  True if this wake was handled
	bool wake_on_send(process *target);

	/// If this process is waiting to receieve from this source, wake it
	/// \argument source  The process that is sending
	/// \returns  True if this wake was handled
	bool wake_on_receive(process *source);

private:
	friend class scheduler;

	/// Set up a new kernel stack for this process, optionally copying the
	/// given stack. Sets the kernel stack on the process object, but also
	/// returns it.
	/// \arg size  Size of the stack to allocate
	/// \arg orig  Address of a stack to copy, or 0 for no copying.
	/// \returns   The address of the new stack as a pointer
	void * setup_kernel_stack(size_t size, uintptr_t orig);
};

using process_list = kutil::linked_list<process>;
using process_node = process_list::item_type;
