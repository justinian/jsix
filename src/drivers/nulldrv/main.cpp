#include <stdint.h>
#include <stdlib.h>

#include "j6/types.h"
#include "j6/errors.h"
#include "j6/signals.h"

#include <j6libc/syscalls.h>

#include "io.h"
#include "serial.h"

char inbuf[1024];
j6_handle_t sys = j6_handle_invalid;
j6_handle_t endp = j6_handle_invalid;

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
	j6_tag_t tag = 0;
	j6_status_t result = _syscall_endpoint_receive(endp, &tag, &len, (void*)buffer);
	if (result != j6_status_ok)
		_syscall_thread_exit(result);

	_syscall_system_log("sub thread received message");

	for (int i = 0; i < len; ++i)
		if (buffer[i] >= 'A' && buffer[i] <= 'Z')
			buffer[i] += 0x20;

	tag++;
	result = _syscall_endpoint_send(endp, tag, len, (void*)buffer);
	if (result != j6_status_ok)
		_syscall_thread_exit(result);

	_syscall_system_log("sub thread sent message");

	for (int i = 1; i < 5; ++i)
		_syscall_thread_sleep(i*10);

	_syscall_system_log("sub thread exiting");
	_syscall_thread_exit(0);
}

void
_init_libc(j6_process_init *init)
{
	sys = init->handles[0];
}

int
main(int argc, const char **argv)
{
	j6_handle_t child = j6_handle_invalid;
	j6_signal_t out = 0;

	_syscall_system_log("main thread starting");

	void *base = malloc(0x1000);
	if (!base)
		return 1;

	uint64_t *vma_ptr = reinterpret_cast<uint64_t*>(base);
	for (int i = 0; i < 3; ++i)
		vma_ptr[i*100] = uint64_t(i);

	_syscall_system_log("main thread wrote to memory area");

	j6_status_t result = _syscall_endpoint_create(&endp);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread created endpoint");

	result = _syscall_thread_create(reinterpret_cast<void*>(&thread_proc), &child);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread created sub thread");

	char message[] = "MAIN THREAD SUCCESSFULLY CALLED SENDRECV IF THIS IS LOWERCASE";
	size_t size = sizeof(message);
	j6_tag_t tag = 16;
	result = _syscall_endpoint_sendrecv(endp, &tag, &size, (void*)message);
	if (result != j6_status_ok)
		return result;

	if (tag != 17)
		_syscall_system_log("GOT WRONG TAG FROM SENDRECV");

	result = _syscall_system_bind_irq(sys, endp, 3);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log(message);

	_syscall_system_log("main thread waiting on child");

	result = _syscall_object_wait(child, -1ull, &out);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main testing irqs");


	serial_port com2(COM2);

	const char *fgseq = "\x1b[2J";
	while (*fgseq)
		com2.write(*fgseq++);

	for (int i = 0; i < 10; ++i)
		com2.write('%');

	size_t len = 0;
	while (true) {
		result = _syscall_endpoint_receive(endp, &tag, &len, nullptr);
		if (result != j6_status_ok)
			return result;

		if (j6_tag_is_irq(tag))
			_syscall_system_log("main thread got irq!");
	}

	_syscall_system_log("main thread closing endpoint");

	result = _syscall_object_close(endp);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread done, exiting");
	return 0;
}

