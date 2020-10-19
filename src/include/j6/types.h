#pragma once
/// \file types.h
/// Basic kernel types exposed to userspace

#include <stdint.h>

/// All interactable kernel objects have a uniqe kernel object id
typedef uint64_t j6_koid_t;

/// Syscalls return status as this type
typedef uint64_t j6_status_t;

/// Some objects have signals, which are a bitmap of 64 possible signals
typedef uint64_t j6_signal_t;

/// The first word of IPC messages are the tag. Tags with the high bit
/// set are reserved for the system.
typedef uint64_t j6_tag_t;

#define j6_tag_system_flag    0x8000000000000000

/// If all high bits except the last 16 are set, then the tag represents
/// an IRQ.
#define j6_tag_irq_base       0xffffffffffff0000
#define j6_tag_is_irq(x)      (((x) & j6_tag_irq_base) == j6_tag_irq_base)
#define j6_tag_from_irq(x)    ((x) | j6_tag_irq_base)
#define j6_tag_to_irq(x)      ((x) & ~j6_tag_irq_base)

/// Handles are references and capabilities to other objects. The least
/// significant 32 bits are an identifier, and the most significant 32
/// bits are a bitmask of capabilities this handle has on that object.
typedef uint64_t j6_handle_t;

#define j6_handle_rights_shift 4
#define j6_handle_id_mask 0xffffffffull
#define j6_handle_invalid 0xffffffffull

/// A process' initial data structure for communicating with the system
struct j6_process_init
{
	j6_handle_t process;
	j6_handle_t handles[3];
};

/// A thread's initial data structure
struct j6_thread_init
{
	j6_handle_t thread;
};
