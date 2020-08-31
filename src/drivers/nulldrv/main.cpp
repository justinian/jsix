#include <stdint.h>
#include <stdlib.h>

#include "j6/types.h"
#include "j6/errors.h"
#include "j6/signals.h"

#include <j6libc/syscalls.h>

const char message[] = "Hello! This is a message being sent over a channel!\n";
char inbuf[1024];
j6_handle_t chan = j6_handle_invalid;

j6_process_init *init = nullptr;

extern "C" {
	void _init_libc(j6_process_init *);
	int main(int, const char **);
}

void
thread_proc()
{
	_syscall_system_log("sub thread starting");

	j6_status_t result = _syscall_object_signal(chan, j6_signal_user0);
	if (result != j6_status_ok)
		_syscall_thread_exit(result);

	_syscall_system_log("sub thread signaled user0");

	size_t size = sizeof(message);
	result = _syscall_channel_send(chan, &size, (void*)message);
	if (result != j6_status_ok)
		_syscall_thread_exit(result);

	_syscall_system_log("sub thread sent on channel");

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

	j6_status_t result = _syscall_channel_create(&chan);
	if (result != j6_status_ok)
		return result;

	size_t size = sizeof(message);
	result = _syscall_channel_send(init->output, &size, (void*)message);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread created channel");

	result = _syscall_thread_create(reinterpret_cast<void*>(&thread_proc), &child);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread waiting on user0");

	result = _syscall_object_wait(chan, j6_signal_user0, &out);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread waiting on can_recv");

	result = _syscall_object_wait(chan, j6_signal_channel_can_recv, &out);
	if (result != j6_status_ok)
		return result;

	size_t len = sizeof(inbuf);
	result = _syscall_channel_receive(chan, &len, (void*)inbuf);
	if (result != j6_status_ok)
		return result;

	for (int i = 0; i < sizeof(message); ++i) {
		if (inbuf[i] != message[i])
			return 127;
	}

	_syscall_system_log("main thread received on channel");

	_syscall_system_log("main thread waiting on child");

	result = _syscall_object_wait(child, -1ull, &out);
	if (result != j6_status_ok)
		return result;

	result = _syscall_channel_close(chan);
	if (result != j6_status_ok)
		return result;

	_syscall_system_log("main thread closed channel");

	_syscall_system_log("main thread done, exiting");
	return 0;
}

