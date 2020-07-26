#include <stdint.h>
#include <stdlib.h>

#include "j6/types.h"
#include "j6/errors.h"
#include "j6/signals.h"

extern "C" {
	j6_status_t system_log(const char *msg);

	j6_status_t object_wait(j6_handle_t obj, j6_signal_t sig, j6_signal_t *out);

	j6_status_t process_koid(j6_koid_t *koid);

	j6_status_t thread_koid(j6_koid_t *koid);
	j6_status_t thread_create(void (*koid)(), j6_handle_t *handle);
	j6_status_t thread_sleep(uint64_t til);
	j6_status_t thread_exit(int64_t status);

	int main(int, const char **);
}

void
thread_proc()
{
	system_log("sub thread starting");
	for (int i = 1; i < 5; ++i)
		thread_sleep(i*10);

	system_log("sub thread exiting");
	thread_exit(0);
}

int
main(int argc, const char **argv)
{
	j6_handle_t child = 0;
	j6_signal_t out = 0;

	system_log("main thread starting");

	j6_status_t result = thread_create(&thread_proc, &child);
	if (result != j6_status_ok)
		return result;

	system_log("main thread waiting on child");

	result = object_wait(child, -1ull, &out);
	if (result != j6_status_ok)
		return result;

	system_log("main thread done, exiting");
	return 0;
}

