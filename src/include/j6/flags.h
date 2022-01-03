#pragma once
/// \file flags.h
/// Enums used as flags for syscalls

enum j6_vm_flags {
#define VM_FLAG(name, v) j6_vm_flag_ ## name = v,
#include <j6/tables/vm_flags.inc>
#undef VM_FLAG
    j6_vm_flag_MAX
};
