#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <arch/memory.h>
#include <j6/cap_flags.h>
#include <j6/channel.hh>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/init.h>
#include <j6/protocols/service_locator.hh>
#include <j6/ring_buffer.hh>
#include <j6/syscalls.h>
#include <j6/sysconf.h>
#include <j6/syslog.hh>
#include <j6/types.h>
#include <util/hash.h>

#include "io.h"
#include "serial.h"

extern "C" {
    int main(int, const char **, const char **envp);
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
main(int argc, const char **argv, const char **envp)
{
    j6::syslog(j6::logs::srv, j6::log_level::info, "uart driver starting");

    j6_handle_t event = j6_handle_invalid;
    j6_status_t result = j6_status_ok;

    g_handle_sys = j6_find_init_handle(0);
    if (g_handle_sys == j6_handle_invalid)
        return 1;

    result = j6_system_request_iopl(g_handle_sys, 3);
    if (result != j6_status_ok)
        return result;

    static constexpr size_t buffer_pages = 1;
    j6::ring_buffer in_buffer {buffer_pages};
    if (!in_buffer.valid())
        return 128;

    j6::ring_buffer out_buffer {buffer_pages};
    if (!out_buffer.valid())
        return 129;

    j6_handle_t slp = j6_find_init_handle(j6::proto::sl::id);
    if (slp == j6_handle_invalid)
        return 1;

    j6::channel *cout = j6::channel::create(0x2000);
    if (!cout)
        return 2;

    uint64_t proto_id = "jsix.protocol.stream.ouput"_id;
    j6::channel_def chan = cout->remote_def();
    j6::proto::sl::client slp_client {slp};

    j6_handle_t chan_handles[2] = {chan.tx, chan.rx};
    result = slp_client.register_service(proto_id, {chan_handles, 2});
    if (result != j6_status_ok)
        return 4;

    result = j6_event_create(&event);
    if (result != j6_status_ok)
        return result;

    result = j6_system_bind_irq(g_handle_sys, event, 3, 0);
    if (result != j6_status_ok)
        return result;

    result = j6_system_bind_irq(g_handle_sys, event, 4, 1);
    if (result != j6_status_ok)
        return result;

    serial_port com1 {COM1, *cout};
    //serial_port com2 {COM2, *cout};

    while (true) {
        uint64_t signals = 0;
        result = j6_event_wait(event, &signals, 500);
        if (result == j6_err_timed_out) {
            com1.do_read();
            com1.do_write();
            //com2.handle_interrupt();
            continue;
        }

        if (result != j6_status_ok) {
            j6::syslog(j6::logs::srv, j6::log_level::error, "uart driver: error %lx waiting for irq", result);
            continue;
        } else {
            j6::syslog(j6::logs::srv, j6::log_level::spam, "uart driver: irq signals: %lx", signals);
        }

        if (signals & (1<<0))
            //com2.handle_interrupt();
        if (signals & (1<<1))
            com1.handle_interrupt();
    }

    j6::syslog(j6::logs::srv, j6::log_level::error, "uart driver somehow got to the end of main");
    return 0;
}
