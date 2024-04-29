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

class channel_source :
    public edit::source
{
public:
    channel_source(j6::channel &chan) : m_chan {chan} {}

    size_t read(char const **data) override {
        return m_chan.get_block(reinterpret_cast<uint8_t const**>(data));
    }

    void consume(size_t size) override { return m_chan.consume(size); }

    void write(char const *data, size_t len) override {
        uint8_t *outp;
        m_chan.reserve(len, &outp);
        memcpy(outp, data, len);
        m_chan.commit(len);
    }

private:
    j6::channel &m_chan;
};


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

    channel_source source {*cout};
    edit::line editor {source};

    static constexpr size_t bufsize = 256;
    char buffer [bufsize];

    while (true) {
        size_t len = editor.read(buffer, bufsize, prompt, prompt_len);
        j6::syslog(j6::logs::app, j6::log_level::info, "Command: %s", buffer);
    }
}
