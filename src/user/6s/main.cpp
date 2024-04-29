#include <string.h>

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

const char greeting[] = "\x1b[2J\x1b[H\x1b[1;30m\
\r\n Welcome to the \x1b[34mjsix\x1b[30m Operating System!\
\r\n \
\r\n This is the \x1b[34m6s\x1b[30m command shell, type `\x1b[4;33mhelp\x1b[0;1;30m`\
\r\n for a list of commands.\
\r\n\r\n\x1b[0m";

static constexpr size_t greeting_len = sizeof(greeting) - 1;

void handle_command(edit::source &s, const char *command, size_t len);

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

    source.write(greeting, greeting_len);

    while (true) {
        size_t len = editor.read(buffer, bufsize, prompt, prompt_len);
        handle_command(source, buffer, len);
    }
}

void
handle_command(edit::source &s, const char *command, size_t len)
{
    j6::syslog(j6::logs::app, j6::log_level::info, "Command: %s", command);
    if (len == 0)
        return;

    if (strncmp(command, "help", len) == 0) {
        s.write("help", 4);
        s.write("\r\n", 2);
        s.write("version", 7);
        s.write("\r\n", 2);
    } else if (!strncmp(command, "version", len)) {
        static const char *version = GIT_VERSION;
        s.write(version, strlen(version));
        s.write("\r\n", 2);
    }
}
