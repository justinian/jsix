#pragma once

#include <stdint.h>
#include "j6/types.h"

struct cpu_state;

enum class syscall : uint64_t
{
#define SYSCALL(id, name, ...) name = id,
#include "j6/tables/syscalls.inc"
#undef SYSCALL
};

void syscall_initialize();
extern "C" void syscall_enable();

namespace syscalls
{
#define SYSCALL(id, name, ...) j6_status_t name (__VA_ARGS__);
#include "j6/tables/syscalls.inc"
#undef SYSCALL
}
