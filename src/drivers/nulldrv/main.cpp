#include <stdint.h>
#include <stdlib.h>

#include "j6/types.h"
#include "j6/errors.h"
#include "j6/signals.h"

#include <j6libc/syscalls.h>

char inbuf[1024];
j6_handle_t endp = j6_handle_invalid;

j6_process_init *init = nullptr;

extern "C" {
	void _init_libc(j6_process_init *);
	int main(int, const char **);
}

void
thread_proc()
{
	_syscall_system_log("sub thread starting");

	char buffer[512];
	size_t len = sizeof(buffer);
	j6_status_t result = _syscall_endpoint_receive(endp, &len, (void*)buffer);
	if (result != j6_status_ok)
		_syscall_thread_exit(result);

	_syscall_system_log("sub thread received message");

	for (int i = 0; i < len; ++i)
		if (buffer[i] >= 'A' && buffer[i] <= 'Z')
			buffer[i] += 0x20;

	result = _syscall_endpoint_send(endp, len, (void*)buffer);
	if (result != j6_status_ok)
		_syscall_thread_exit(result);

	_syscall_system_log("sub thread sent message");

	for (int i = 1; i < 5; ++i)
		_syscall_thread_sleep(i*10);

	_syscall_system_log("sub thread exiting");
	_syscall_thread_exit(0);
}

void
_init_libc(j6_process_init *i)
{
	init = i;
}

int
main(int argc, const char **argv)
{
	j6_handle_t child = j6_handle_invalid;
	j6_signal_t out = 0;

	_syscall_system_log("main thread starting");

	uintptr_t base = 0xcc0000000;
	j6_handle_t vma = j6_handle_invalid;
	j6_status_t result = _syscall_vma_create_map(&vma, 0x100000, base);
	if (result != j6_status_ok)
		return result;

	uint64_t *vma_ptr = reinterpret_cast<uint64_t*>(base);
	for (int i = 0; i < 4096; ++i)
		vma_ptr[i] = uint64_t(i);

	result = _syscall_endpoint_create(&endp);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread created endpoint");

	result = _syscall_thread_create(reinterpret_cast<void*>(&thread_proc), &child);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread created sub thread");

	char message[] = "MAIN THREAD SUCCESSFULLY CALLED SENDRECV IF THIS IS LOWERCASE";
	size_t size = sizeof(message);
	result = _syscall_endpoint_sendrecv(endp, &size, (void*)message);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log(message);

	_syscall_system_log("main thread waiting on child");

	result = _syscall_object_wait(child, -1ull, &out);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread closing endpoint");

	result = _syscall_endpoint_close(endp);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread done, exiting");
	return 0;
}

