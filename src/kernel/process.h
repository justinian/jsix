#pragma once
/// \file process.h
/// The processes and related definitions
#include "kutil/enum_bitfields.h"
#include "kutil/linked_list.h"
#include "kutil/memory.h"
#include "page_manager.h"


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
	time
};

/// A process
struct process
{
	uint32_t pid;
	uint32_t ppid;

	process_flags flags;

	uint16_t quanta;

	uint8_t priority;

	process_wait waiting;
	uint64_t waiting_info;

	uint32_t return_code;

	uint32_t reserved1;

	addr_t rsp;
	page_table *pml4;

	/// Unready this process until it gets a signal
	/// \arg sigmask  A bitfield of signals to wake on
	void wait_on_signal(uint64_t sigmask);

	/// Unready this process until a child exits
	/// \arg pid  PID of the child to wait for, or 0 for any
	void wait_on_child(uint32_t pid);

	/// Unready this process until after the given time
	/// \arg time  The time after which to wake
	void wait_on_time(uint64_t time);

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
};

using process_list = kutil::linked_list<process>;
using process_node = process_list::item_type;
