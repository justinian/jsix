#include <new>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <j6/channel.hh>
#include <j6/cap_flags.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/init.h>
#include <j6/protocols/service_locator.hh>
#include <j6/syscalls.h>
#include <j6/sysconf.h>
#include <j6/types.h>
#include <util/hash.h>

extern "C" {
    int main(int, const char **);
}

extern j6_handle_t __handle_self;
j6_handle_t g_handle_sys = j6_handle_invalid;

static const uint8_t level_colors[] = {0x00, 0x09, 0x01, 0x0b, 0x0f, 0x07, 0x08};
char const * const level_names[] = {"", "fatal", "error", "warn", "info", "verbose", "spam"};
char const * const area_names[] = {
#define LOG(name, lvl) #name ,
#include <j6/tables/log_areas.inc>
#undef LOG
    nullptr
};

void
print_header(j6::channel *cout)
{
    char stringbuf[150];

    unsigned version_major = j6_sysconf(j6sc_version_major);
    unsigned version_minor = j6_sysconf(j6sc_version_minor);
    unsigned version_patch = j6_sysconf(j6sc_version_patch);
    unsigned version_git = j6_sysconf(j6sc_version_gitsha);

    size_t len = snprintf(stringbuf, sizeof(stringbuf),
            "\e[38;5;21mjsix OS\e[38;5;8m %d.%d.%d (%07x) booting...\e[0m\r\n",
            version_major, version_minor, version_patch, version_git);

    cout->send(stringbuf, len);
}

void
log_pump_proc(j6::channel *cout)
{
    size_t buffer_size = 0;
    void *message_buffer = nullptr;
    char stringbuf[300];

    j6_status_t result = j6_system_request_iopl(g_handle_sys, 3);
    if (result != j6_status_ok)
        return;

    uint64_t seen = 0;

    while (true) {
        size_t size = buffer_size;
        j6_status_t s = j6_system_get_log(g_handle_sys, seen, message_buffer, &size);

        if (s == j6_err_insufficient) {
            free(message_buffer);
            buffer_size = size * 2;
            message_buffer = malloc(buffer_size);
            continue;
        } else if (s != j6_status_ok) {
            j6_log("uart driver got error from get_log");
            continue;
        }

        const j6_log_entry *e = reinterpret_cast<j6_log_entry*>(message_buffer);

        seen = e->id;
        const char *area_name = area_names[e->area];
        const char *level_name = level_names[e->severity];
        uint8_t level_color = level_colors[e->severity];

        int message_len = static_cast<int>(e->bytes - sizeof(j6_log_entry));
        size_t len = snprintf(stringbuf, sizeof(stringbuf),
                "\e[38;5;%dm%5lx %7s %7s: %.*s\e[38;5;0m\r\n",
                level_color, seen, area_name, level_name,
                message_len, e->message);
        cout->send(stringbuf, len+1);
    }
}

int
main(int argc, const char **argv)
{
    j6_log("logging server starting");

    g_handle_sys = j6_find_init_handle(0);
    if (g_handle_sys == j6_handle_invalid)
        return 1;

    j6_handle_t slp = j6_find_init_handle(j6::proto::sl::id);
    if (g_handle_sys == j6_handle_invalid)
        return 2;

    j6_handle_t cout_vma = j6_handle_invalid;

    uint64_t proto_id = "jsix.protocol.stream.ouput"_id;
    j6::proto::sl::client slp_client {slp};

    for (unsigned i = 0; i < 100; ++i) {
        j6_status_t s = slp_client.lookup_service(proto_id, cout_vma);
        if (s == j6_status_ok &&
            cout_vma != j6_handle_invalid)
            break;

        cout_vma = j6_handle_invalid;
        j6_thread_sleep(10000); // 10ms
    }

    if (cout_vma == j6_handle_invalid)
        return 3;

    j6::channel *cout = j6::channel::open(cout_vma);
    if (!cout)
        return 4;

    print_header(cout);
    log_pump_proc(cout);
    return 0;
}

