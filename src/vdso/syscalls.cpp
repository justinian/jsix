#include <j6/errors.h>
#include <j6/types.h>
#include "vdso_internal.h"

#define SYSCALL(num, name, ...) \
	j6_status_t __sys_j6_##name (__VA_ARGS__) { \
		j6_status_t result = 0; \
		__asm__ __volatile__ ( "syscall" : "=a"(result) : "a"(num) ); \
		return result; \
	} \
	j6_status_t j6_ ## name(__VA_ARGS__) __weak_alias("__sys_j6_" #name);

extern "C" {
#include "syscalls.inc"
}

#undef SYSCALL
