#include <stdint.h>
#include <stdlib.h>

#include "j6/types.h"
#include "j6/errors.h"
#include "j6/signals.h"

const char message[] = "Hello! This is a message being sent over a channel!";
char inbuf[1024];
j6_handle_t chan = j6_handle_invalid;

extern "C" {
	j6_status_t system_log(const char *msg);

	j6_status_t object_wait(j6_handle_t obj, j6_signal_t sig, j6_signal_t *out);

	j6_status_t process_koid(j6_koid_t *koid);

	j6_status_t thread_koid(j6_koid_t *koid);
	j6_status_t thread_create(void (*koid)(), j6_handle_t *handle);
	j6_status_t thread_sleep(uint64_t til);
	j6_status_t thread_exit(int64_t status);

	j6_status_t channel_create(j6_handle_t *handle);
	j6_status_t channel_close(j6_handle_t handle);
	j6_status_t channel_send(j6_handle_t handle, size_t len, void *data);
	j6_status_t channel_receive(j6_handle_t handle, size_t *len, void *data);

	int main(int, const char **);
}

void
thread_proc()
{
	system_log("sub thread starting");

	j6_status_t result = channel_send(chan, sizeof(message), (void*)message);
	if (result != j6_status_ok)
		thread_exit(result);

	system_log("sub thread sent on channel");

	for (int i = 1; i < 5; ++i)
		thread_sleep(i*10);

	system_log("sub thread exiting");
	thread_exit(0);
}

int
main(int argc, const char **argv)
{
	j6_handle_t child = j6_handle_invalid;
	j6_signal_t out = 0;

	system_log("main thread starting");

	j6_status_t result = channel_create(&chan);
	if (result != j6_status_ok)
		return result;

	system_log("main thread created channel");

	result = thread_create(&thread_proc, &child);
	if (result != j6_status_ok)
		return result;

	result = object_wait(chan, j6_signal_channel_can_recv, &out);
	if (result != j6_status_ok)
		return result;

	size_t len = sizeof(inbuf);
	result = channel_receive(chan, &len, (void*)inbuf);
	if (result != j6_status_ok)
		return result;

	for (int i = 0; i < sizeof(message); ++i) {
		if (inbuf[i] != message[i])
			return 127;
	}

	system_log("main thread received on channel");

	system_log("main thread waiting on child");

	result = object_wait(child, -1ull, &out);
	if (result != j6_status_ok)
		return result;

	result = channel_close(chan);
	if (result != j6_status_ok)
		return result;

	system_log("main thread closed channel");

	system_log("main thread done, exiting");
	return 0;
}

