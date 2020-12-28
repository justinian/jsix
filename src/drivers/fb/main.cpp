#include <stdint.h>
#include <stdlib.h>

#include "j6/types.h"
#include "j6/errors.h"
#include "j6/signals.h"

#include <j6libc/syscalls.h>

extern "C" {
	void _init_libc(j6_process_init *);
	int main(int, const char **);
}

j6_handle_t sys = j6_handle_invalid;
size_t size = 0;

void
_init_libc(j6_process_init *init)
{
	sys = init->handles[0];
	size = reinterpret_cast<size_t>(init->handles[1]);
}

int
main(int argc, const char **argv)
{
	_syscall_system_log("fb driver starting");

	if (size == 0)
		return 1;

	uint32_t *fb = reinterpret_cast<uint32_t*>(0x100000000);
	for (size_t i=0; i < size/4; ++i) {
		fb[i] = 0xff;
	}

	_syscall_system_log("fb driver done, exiting");
	return 0;
}

