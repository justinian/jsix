#include <new>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <j6/cap_flags.h>
#include <j6/channel.hh>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/init.h>
#include <j6/protocols/service_locator.hh>
#include <j6/syscalls.h>
#include <j6/sysconf.h>
#include <j6/syslog.hh>
#include <j6/types.h>
#include <util/hash.h>

#include "io.h"
#include "serial.h"

extern "C" {
    int main(int, const char **);
}

j6_handle_t g_handle_sys = j6_handle_invalid;

constexpr size_t in_buf_size = 512;
constexpr size_t out_buf_size = 128 * 1024;

uint8_t com1_in[in_buf_size];
uint8_t com2_in[in_buf_size];
uint8_t com1_out[out_buf_size];
uint8_t com2_out[out_buf_size];

constexpr uintptr_t stack_top = 0xf80000000;

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

    static constexpr size_t buffer_size = 512;
    char buffer[buffer_size];

    j6_handle_t slp = j6_find_first_handle(j6_object_type_mailbox);
    if (slp == j6_handle_invalid)
        return 1;

    j6::channel *cout = j6::channel::create(0x2000);
    if (!cout)
        return 2;

    uint64_t proto_id = "jsix.protocol.stream.ouput"_id;
    uint64_t handle = cout->handle();
    j6::proto::sl::client slp_client {slp};
    result = slp_client.register_service(proto_id, handle);
    if (result != j6_status_ok)
        return 4;

    result = j6_system_request_iopl(g_handle_sys, 3);
    if (result != j6_status_ok)
        return 6;

    while (true) {
        size_t size = buffer_size;
        while (true) {
            result = cout->receive(buffer, &size, 0);
            if (result != j6_status_ok)
                break;

            //j6::syslog("uart driver: got %d bytes from channel", size);
            com1.write(buffer, size);
        }
        if (result != j6_status_would_block)
            j6::syslog("uart driver: error %lx receiving from channel", result);

        uint64_t signals = 0;
        result = j6_event_wait(event, &signals, 500);
        if (result == j6_err_timed_out) {
            com1.do_write();
            continue;
        }

        if (result != j6_status_ok) {
            j6::syslog("uart driver: error %lx waiting for irq", result);
            continue;
        } else {
            j6::syslog("uart driver: irq signals: %lx", signals);
        }

        if (signals & (1<<0))
            com2.handle_interrupt();
        if (signals & (1<<1))
            com1.handle_interrupt();
    }

    j6_log("uart driver somehow got to the end of main");
    return 0;
}

