#include <new>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <j6/caps.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/init.h>
#include <j6/protocols/service_locator.h>
#include <j6/syscalls.h>
#include <j6/sysconf.h>
#include <j6/types.h>
#include <util/hash.h>

#include "io.h"
#include "serial.h"

extern "C" {
    int main(int, const char **);
}

extern j6_handle_t __handle_self;
j6_handle_t g_handle_sys = j6_handle_invalid;

constexpr size_t in_buf_size = 512;
constexpr size_t out_buf_size = 128 * 1024;

uint8_t com1_in[in_buf_size];
uint8_t com2_in[in_buf_size];
uint8_t com1_out[out_buf_size];
uint8_t com2_out[out_buf_size];

serial_port *g_com1;
serial_port *g_com2;

constexpr size_t stack_size = 0x10000;
constexpr uintptr_t stack_top = 0xf80000000;

int
channel_pump_loop()
{
    j6_status_t result;
    constexpr size_t buffer_size = 512;
    char buffer[buffer_size];

    j6_handle_t slp = j6_find_first_handle(j6_object_type_mailbox);
    if (slp == j6_handle_invalid)
        return 1;

    j6_handle_t cout = j6_handle_invalid;
    result = j6_channel_create(&cout);
    if (result != j6_status_ok)
        return 2;

    j6_handle_t cout_write = j6_handle_invalid;
    result = j6_handle_clone(cout, &cout_write,
            j6_cap_channel_send | j6_cap_object_clone);
    if (result != j6_status_ok)
        return 3;

    uint64_t tag = j6_proto_sl_register;
    uint64_t data = "jsix.protocol.stream.ouput"_id;
    size_t data_len = sizeof(data);
    size_t handle_count = 1;
    result = j6_mailbox_call(slp, &tag,
            &data, &data_len, 
            &cout_write, &handle_count);
    if (result != j6_status_ok)
        return 4;

    if (tag != j6_proto_base_status || data != j6_status_ok)
        return 5;

    result = j6_system_request_iopl(g_handle_sys, 3);
    if (result != j6_status_ok)
        return 6;

    while (true) {
        size_t size = buffer_size;
        j6_status_t s = j6_channel_receive(cout, buffer, &size, j6_channel_block);
        if (s == j6_status_ok)
            g_com1->write(buffer, size);
    }
}

void
pump_proc()
{
    j6_thread_exit(channel_pump_loop());
}

int
main(int argc, const char **argv)
{
    j6_log("uart driver starting");

    j6_handle_t event = j6_handle_invalid;
    j6_status_t result = j6_status_ok;

    g_handle_sys = j6_find_first_handle(j6_object_type_system);
    if (g_handle_sys == j6_handle_invalid)
        return 1;

    result = j6_system_request_iopl(g_handle_sys, 3);
    if (result != j6_status_ok)
        return result;

    result = j6_event_create(&event);
    if (result != j6_status_ok)
        return result;

    result = j6_system_bind_irq(g_handle_sys, event, 3, 0);
    if (result != j6_status_ok)
        return result;

    result = j6_system_bind_irq(g_handle_sys, event, 4, 1);
    if (result != j6_status_ok)
        return result;

    serial_port com1 {COM1, in_buf_size, com1_in, out_buf_size, com1_out};
    serial_port com2 {COM2, in_buf_size, com2_in, out_buf_size, com2_out};
    g_com1 = &com1;
    g_com2 = &com2;

    j6_handle_t child_stack_vma = j6_handle_invalid;
    result = j6_vma_create_map(&child_stack_vma, stack_size, stack_top-stack_size, j6_vm_flag_write);
    if (result != j6_status_ok)
        return result;

    uint64_t *sp = reinterpret_cast<uint64_t*>(stack_top - 0x10);
    sp[0] = sp[1] = 0;

    j6_handle_t child = j6_handle_invalid;
    result = j6_thread_create(&child, __handle_self, stack_top - 0x10, reinterpret_cast<uintptr_t>(&pump_proc));
    if (result != j6_status_ok)
        return result;

    size_t len = 0;
    while (true) {
        uint64_t signals = 0;
        result = j6_event_wait(event, &signals, 100);
        if (result == j6_err_timed_out) {
            com1.handle_interrupt();
            com2.handle_interrupt();
            continue;
        }

        if (result != j6_status_ok) {
            j6_log("uart driver got error waiting for irq");
            continue;
        }

        if (signals & (1<<0))
            com2.handle_interrupt();
        if (signals & (1<<1))
            com1.handle_interrupt();
    }

    j6_log("uart driver somehow got to the end of main");
    return 0;
}

