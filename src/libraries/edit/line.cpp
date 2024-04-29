#include <edit/line.h>
#include <j6/memutils.h>
#include <j6/syslog.hh>

static inline size_t min(size_t a, size_t b) { return a > b ? a : b; }

namespace edit {

static constexpr char ESC = '\x1b';
static constexpr char BACKSPACE = '\x08';
static constexpr char DEL = '\x7f';

static const char init_line[] = "\x1b[999D\x1b[K";
static const size_t init_line_len = sizeof(init_line) - 1;

static const char get_pos[] = "\x1b[n";
static const size_t get_pos_len = sizeof(get_pos) - 1;

static const char backspace[] = "\x1b[D\x1b[K";
static const size_t backspace_len = sizeof(backspace) - 1;

line::line(source &s) :
    m_source {s}
{
}


line::~line()
{
}


size_t
line::read(char *buffer, size_t size, char const *prompt, size_t prompt_len)
{
    m_source.write(init_line, init_line_len);
    if (prompt && prompt_len)
        m_source.write(prompt, prompt_len);

    size_t in = 0;

    while (in < size) {
        char const *input = nullptr;
        size_t inlen = m_source.read(&input);

        size_t i = 0;
        size_t sub_len = 0;
        while (i < inlen) {
            switch (input[i]) {
            case ESC:
                sub_len = parse_command(input + i, inlen - i);
                if (!sub_len)
                    goto reread;
                i += sub_len;
                break;

            case BACKSPACE:
            case DEL:
                if (in > 0) {
                    --in;
                    m_source.write(backspace, backspace_len);
                }
                ++i;
                break;

            case '\r':
                m_source.consume(++i);
                m_source.write("\n", 1);
                buffer[in] = 0;
                return in;

            default:
                m_source.write(input + i, 1);
                buffer[in++] = input[i++];
                break;
            }
        }

reread:
        m_source.consume(i);
    }

    return 0;
}


size_t
line::parse_command(char const *data, size_t len)
{
    size_t i = 2;

    char buf[60];
    char *p = buf;

    while (i < len) {
        if ((data[i] >= '0' && data[i] <= '9') || data[i] == ';') {
            *p++ = data[i];
        } else {
            *p++ = data[i];
            *p = 0;
            j6::syslog(j6::logs::app, j6::log_level::verbose,
                    "line edit got command: ESC[%s", buf);
            return i;
        }

        ++i;
    }
    return 0;
}

} // namespace line
