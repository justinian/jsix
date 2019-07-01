#pragma once
/// \file types.h
/// Basic kernel types exposed to userspace

#include <stdint.h>

/// All interactable kernel objects have a uniqe kernel object id
typedef uint64_t j6_koid_t;

/// Syscalls return status as this type
typedef uint32_t j6_status_t;

/// Handles are references and capabilities to other objects
typedef uint32_t j6_handle_t;

/// Some objects have signals, which are a bitmap of 64 possible signals
typedef uint64_t j6_signal_t;

/// The rights of a handle/capability are a bitmap of 64 possible rights
typedef uint64_t j6_rights_t;
