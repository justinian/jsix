#include <stdint.h>
#include <stdlib.h>

#include "j6/types.h"
#include "j6/errors.h"
#include "j6/signals.h"
#include "j6/syscalls.h"

#include "io.h"
#include "serial.h"

char inbuf[1024];
extern j6_handle_t __handle_sys;
j6_handle_t endp = j6_handle_invalid;

extern "C" {
	int main(int, const char **);
}

void
thread_proc()
{
	j6_system_log("sub thread starting");

	char buffer[512];
	size_t len = sizeof(buffer);
	j6_tag_t tag = 0;
	j6_status_t result = j6_endpoint_receive(endp, &tag, &len, (void*)buffer);
	if (result != j6_status_ok)
		j6_thread_exit(result);

	j6_system_log("sub thread received message");

	for (int i = 0; i < len; ++i)
		if (buffer[i] >= 'A' && buffer[i] <= 'Z')
			buffer[i] += 0x20;

	tag++;
	result = j6_endpoint_send(endp, tag, len, (void*)buffer);
	if (result != j6_status_ok)
		j6_thread_exit(result);

	j6_system_log("sub thread sent message");

	for (int i = 1; i < 5; ++i)
		j6_thread_sleep(i*10);

	j6_system_log("sub thread exiting");
	j6_thread_exit(0);
}

int
main(int argc, const char **argv)
{
	j6_handle_t children[2] = {j6_handle_invalid, j6_handle_invalid};
	j6_signal_t out = 0;

	j6_system_log("main thread starting");

	for (int i = 0; i < argc; ++i)
		j6_system_log(argv[i]);

	void *base = malloc(0x1000);
	if (!base)
		return 1;

	uint64_t *vma_ptr = reinterpret_cast<uint64_t*>(base);
	for (int i = 0; i < 3; ++i)
		vma_ptr[i*100] = uint64_t(i);

	j6_system_log("main thread wrote to memory area");

	j6_status_t result = j6_endpoint_create(&endp);
	if (result != j6_status_ok)
		return result;

	j6_system_log("main thread created endpoint");

	result = j6_thread_create(reinterpret_cast<void*>(&thread_proc), &children[1]);
	if (result != j6_status_ok)
		return result;

	j6_system_log("main thread created sub thread");

	char message[] = "MAIN THREAD SUCCESSFULLY CALLED SENDRECV IF THIS IS LOWERCASE";
	size_t size = sizeof(message);
	j6_tag_t tag = 16;
	result = j6_endpoint_sendrecv(endp, &tag, &size, (void*)message);
	if (result != j6_status_ok)
		return result;

	if (tag != 17)
		j6_system_log("GOT WRONG TAG FROM SENDRECV");

	result = j6_system_bind_irq(__handle_sys, endp, 3);
	if (result != j6_status_ok)
		return result;

	j6_system_log(message);

	j6_system_log("main thread creating a new process");
	result = j6_process_create(&children[0]);
	if (result != j6_status_ok)
		return result;

	j6_system_log("main thread waiting on children");

	j6_handle_t outhandle;
	result = j6_object_wait_many(children, 2, -1ull, &outhandle, &out);
	if (result != j6_status_ok)
		return result;

	if (outhandle == children[1]) {
		j6_system_log("   ok from child thread");
	} else if (outhandle == children[0]) {
		j6_system_log("   ok from child process");
	} else {
		j6_system_log("   ... got unknown handle");
	}

	j6_system_log("main testing irqs");


	serial_port com2(COM2);

	const char *fgseq = "\x1b[2J";
	while (*fgseq)
		com2.write(*fgseq++);

	for (int i = 0; i < 10; ++i)
		com2.write('%');

	size_t len = 0;
	while (true) {
		result = j6_endpoint_receive(endp, &tag, &len, nullptr);
		if (result != j6_status_ok)
			return result;

		if (j6_tag_is_irq(tag))
			j6_system_log("main thread got irq!");
	}

	j6_system_log("main thread closing endpoint");

	result = j6_object_close(endp);
	if (result != j6_status_ok)
		return result;

	j6_system_log("main thread done, exiting");
	return 0;
}

