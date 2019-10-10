#pragma once
/// \file vdso_internal.h
/// VDSO syscall forward-declares and linker utils

#define __weak_alias(name) __attribute__((weak, alias(name)));
#define __local __attribute__((__visibility__("hidden")))

#define SYSCALL(num, name, ...) \
	j6_status_t __sys_j6_ ## name (__VA_ARGS__) __local; \

extern "C" {
#include "syscalls.inc"
}

#undef SYSCALL

