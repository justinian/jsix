#include <j6/channel.hh>
#include <j6/init.h>
#include <j6/errors.h>
#include <j6/memutils.h>
#include <j6/protocols/service_locator.hh>
#include <j6/ring_buffer.hh>
#include <j6/syscalls.h>
#include <j6/syslog.hh>
#include <j6/types.h>
#include <edit/line.h>

extern "C" {
    int main(int, const char **);
}

j6_handle_t g_handle_sys = j6_handle_invalid;

const char prompt[] = "\x1b[1;32mj6> \x1b[0m";
static constexpr size_t prompt_len = sizeof(prompt) - 1;

const char query[] = "\x1b[6n";
static constexpr size_t query_len = sizeof(prompt) - 1;

size_t
gather_command(j6::channel &cout, j6::ring_buffer &buf)
{
    size_t total_len = buf.used();

    while (true) {
        size_t size = buf.free();
        char *start = buf.write_ptr();

        uint8_t const *input = nullptr;
        size_t n = cout.get_block(&input);
        if (n > size) n = size;

        uint8_t *outp = nullptr;
        cout.reserve(n, &outp, true);

        size_t i = 0;
        bool newline = false;
        bool eof = false;
        while (i < n) {
            start[i] = input[i];
            outp[i] = input[i];
            if (start[i] == 0x1b)
                outp[i] = 'E';
            if (start[i] == 4)
                eof = true;

            if (start[i++] == '\r') {
                newline = true;
                break;
            }
        }

        cout.commit(i);
        cout.consume(i);

        if (eof) {
            cout.reserve(query_len, &outp, true);
            memcpy(outp, query, query_len);
            cout.commit(query_len);
        }

        if (newline)
            return total_len + i;

        total_len += size;
    }
}

int
main(int argc, const char **argv)
{
    j6_handle_t event = j6_handle_invalid;
    j6_status_t result = j6_status_ok;

    g_handle_sys = j6_find_init_handle(0);
    if (g_handle_sys == j6_handle_invalid)
        return 1;

    /*
    static constexpr size_t buffer_pages = 1;
    j6::ring_buffer in_buffer {buffer_pages};
    if (!in_buffer.valid())
        return 2;
    */

    j6_handle_t slp = j6_find_init_handle(j6::proto::sl::id);
    if (slp == j6_handle_invalid)
        return 3;

    uint64_t proto_id = "jsix.protocol.stream.ouput"_id;
    j6::proto::sl::client slp_client {slp};

    j6_handle_t chan_handles[2];
    util::counted<j6_handle_t> handles {chan_handles, 2};

    for (unsigned i = 0; i < 100; ++i) {
        j6_status_t s = slp_client.lookup_service(proto_id, handles);
        if (s == j6_status_ok && handles.count)
            break;

        j6_thread_sleep(10000); // 10ms
    }

    if (!handles.count)
        return 4;

    j6::channel *cout = j6::channel::open({chan_handles[0], chan_handles[1]});
    if (!cout)
        return 5;

    edit::line editor {prompt, prompt_len};

    while (true) {
        uint8_t *outb = nullptr;
        char const *inc = nullptr;
        size_t len = editor.output(&inc);
        while (len) {
            cout->reserve(len, &outb, true);
            memcpy(outb, inc, len);
            cout->commit(len);
            len = editor.output(&inc);
        }

        char *outc = nullptr;
        uint8_t const *inb = nullptr;
        len = cout->get_block(&inb);
        if (len) {
            len = editor.input(reinterpret_cast<const char*>(inb), len);
            cout->consume(len);
        }
    }
}
