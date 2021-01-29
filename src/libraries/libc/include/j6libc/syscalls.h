#pragma once

#include <j6/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYSCALL(n, name, ...) j6_status_t _syscall_ ## name (__VA_ARGS__);
#include "j6/tables/syscalls.inc"
#undef SYSCALL

#ifdef __cplusplus
}
#endif
