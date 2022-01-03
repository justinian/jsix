#include <stdint.h>
#include <stdlib.h>

#include <j6/types.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/signals.h>
#include <j6/syscalls.h>

#include "io.h"
#include "serial.h"

char inbuf[1024];
extern j6_handle_t __handle_sys;
j6_handle_t endp = j6_handle_invalid;

extern "C" {
    int main(int, const char **);
}

constexpr j6_handle_t handle_self   = 1; // Self program handle is always 1

constexpr size_t stack_size = 0x10000;
constexpr uintptr_t stack_top = 0xf80000000;

void
thread_proc()
{
    j6_log("sub thread starting");

    char buffer[512];
    size_t len = sizeof(buffer);
    j6_tag_t tag = 0;
    j6_status_t result = j6_endpoint_receive(endp, &tag, (void*)buffer, &len);
    if (result != j6_status_ok)
        j6_thread_exit(result);

    j6_log("sub thread received message");

    for (int i = 0; i < len; ++i)
        if (buffer[i] >= 'A' && buffer[i] <= 'Z')
            buffer[i] += 0x20;

    tag++;
    result = j6_endpoint_send(endp, tag, (void*)buffer, len);
    if (result != j6_status_ok)
        j6_thread_exit(result);

    j6_log("sub thread sent message");

    for (int i = 1; i < 5; ++i)
        j6_thread_sleep(i*10);

    j6_log("sub thread exiting");
    j6_thread_exit(0);
}

int
main(int argc, const char **argv)
{
    j6_signal_t out = 0;

    j6_log("main thread starting");

    for (int i = 0; i < argc; ++i)
        j6_log(argv[i]);

    void *base = malloc(0x1000);
    if (!base)
        return 1;

    uint64_t *vma_ptr = reinterpret_cast<uint64_t*>(base);
    for (int i = 0; i < 3; ++i)
        vma_ptr[i*100] = uint64_t(i);

    j6_log("main thread wrote to memory area");

    j6_status_t result = j6_endpoint_create(&endp);
    if (result != j6_status_ok)
        return result;

    j6_log("main thread created endpoint");

    j6_handle_t child_stack_vma = j6_handle_invalid;
    result = j6_vma_create_map(&child_stack_vma, stack_size, stack_top-stack_size, j6_vm_flag_write);
    if (result != j6_status_ok)
        return result;

    j6_handle_t child = j6_handle_invalid;
    result = j6_thread_create(&child, handle_self, stack_top, reinterpret_cast<uintptr_t>(&thread_proc));
    if (result != j6_status_ok)
        return result;

    j6_log("main thread created sub thread");

    char message[] = "MAIN THREAD SUCCESSFULLY CALLED SENDRECV IF THIS IS LOWERCASE";
    size_t size = sizeof(message);
    j6_tag_t tag = 16;
    result = j6_endpoint_sendrecv(endp, &tag, (void*)message, &size);
    if (result != j6_status_ok)
        return result;

    if (tag != 17)
        j6_log("GOT WRONG TAG FROM SENDRECV");

    result = j6_system_bind_irq(__handle_sys, endp, 3);
    if (result != j6_status_ok)
        return result;

    j6_log(message);

    j6_log("main thread waiting on sub thread");

    result = j6_kobject_wait(child, -1ull, &out);
    if (result != j6_status_ok)
        return result;

    j6_log("main setting IOPL");

    result = j6_system_request_iopl(__handle_sys, 3);
    if (result != j6_status_ok)
        return result;

    j6_log("main testing irqs");

    serial_port com2(COM2);

    const char *fgseq = "\x1b[2J";
    while (*fgseq)
        com2.write(*fgseq++);

    for (int i = 0; i < 10; ++i)
        com2.write('%');

    size_t len = 0;
    while (true) {
        result = j6_endpoint_receive(endp, &tag, nullptr, &len);
        if (result != j6_status_ok)
            return result;

        if (j6_tag_is_irq(tag))
            j6_log("main thread got irq!");
    }

    j6_log("main thread done, exiting");
    return 0;
}

