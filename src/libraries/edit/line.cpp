#include <edit/line.h>
#include <j6/memutils.h>
#include <j6/syslog.hh>

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

line::line(const char *prompt, size_t prompt_len) :
    m_out {0}
{
    m_buf = new char [1024];
    memcpy(m_buf, init_line, init_line_len);
    m_in = init_line_len;

    if (prompt && prompt_len) {
        memcpy(m_buf + m_in, prompt, prompt_len);
        m_in += prompt_len;
    }

    m_prefix = m_in;

    m_tmp_out = get_pos;
    m_tmp_out_len = get_pos_len;
}


line::~line()
{
    delete [] m_buf;
}


size_t
line::input(char const *data, size_t len)
{
    j6::syslog(j6::logs::app, j6::log_level::spam, "Line edit got %d bytes input", len);
    size_t i = 0;
    size_t sub_len = 0;
    while (i < len) {
        switch (data[i]) {
        case ESC:
            sub_len = parse_command(data + i, len - i);
            if (!sub_len)
                return i; // Not yet enough data
            i += sub_len;
            break;

        case BACKSPACE:
        case DEL:
            if (m_in > m_prefix) {
                --m_in;
                if (m_out > m_in)
                    m_out = m_in;
                m_tmp_out = backspace;
                m_tmp_out_len = backspace_len;
            }
            ++i;
            break;

        case '\r':
            m_out = 0;
            m_in = m_prefix;
            m_tmp_out = "\n";
            m_tmp_out_len = 1;
            ++i;
            break;

        default:
            m_buf[m_in++] = data[i++];
            break;
        }
    }

    return i;
}


size_t
line::output(char const **data)
{
    if (m_tmp_out) {
        char const *out = m_tmp_out;
        m_tmp_out = nullptr;

        size_t len = m_tmp_out_len;
        m_tmp_out_len = 0;

        *data = out;
        j6::syslog(j6::logs::app, j6::log_level::spam, "Line edit sending %d bytes alt output", len);
        return len;
    }

    *data = (m_buf + m_out);
    size_t len = m_in - m_out;
    m_out = m_in;
    j6::syslog(j6::logs::app, j6::log_level::spam, "Line edit sending %d bytes output", len);
    return len;
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
