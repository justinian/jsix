#include "kutil/no_construct.h"
#include "printf/printf.h"

#include "console.h"
#include "serial.h"


const char digits[] = "0123456789abcdef";

static kutil::no_construct<console> __g_console_storage;
console &g_console = __g_console_storage.value;


console::console() :
    m_serial(nullptr)
{
}

console::console(serial_port *serial) :
    m_serial(serial)
{
    if (m_serial) {
        const char *fgseq = "\x1b[2J";
        while (*fgseq)
            m_serial->write(*fgseq++);
    }
}

void
console::echo()
{
    putc(m_serial->read());
}

void
console::set_color(uint8_t fg, uint8_t bg)
{
    if (m_serial) {
        const char *fgseq = "\x1b[38;5;";
        while (*fgseq)
            m_serial->write(*fgseq++);
        if (fg >= 100) m_serial->write('0' + (fg/100));
        if (fg >=  10) m_serial->write('0' + (fg/10) % 10);
        m_serial->write('0' + fg % 10);
        m_serial->write('m');

        const char *bgseq = "\x1b[48;5;";
        while (*bgseq)
            m_serial->write(*bgseq++);
        if (bg >= 100) m_serial->write('0' + (bg/100));
        if (bg >=  10) m_serial->write('0' + (bg/10) % 10);
        m_serial->write('0' + bg % 10);
        m_serial->write('m');
    }
}

size_t
console::puts(const char *message)
{
    size_t n = 0;
    while (message && *message) {
        n++;
        putc(*message++);
    }

    return n;
}

void
console::putc(char c)
{
    if (m_serial) {
        if (c == '\n') m_serial->write('\r');
        m_serial->write(c);
    }
}

void console::vprintf(const char *fmt, va_list args)
{
    static const unsigned buf_size = 256;
    char buffer[buf_size];
    vsnprintf(buffer, buf_size, fmt, args);
    puts(buffer);
}
