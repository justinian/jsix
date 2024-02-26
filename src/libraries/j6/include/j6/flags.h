#pragma once
/// \file flags.h
/// Enums used as flags for syscalls

enum j6_vm_flags {
    j6_vm_flag_none = 0,
#define VM_FLAG(name, v) j6_vm_flag_ ## name = (1ul << v),
#include <j6/tables/vm_flags.inc>
#undef VM_FLAG
    j6_vm_flag_MAX
};

enum j6_flags {
    j6_flag_block = 0x01,

    j6_flags_MAX // custom per-type flags should start here
};
