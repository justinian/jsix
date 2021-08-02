#pragma once
#include <stdarg.h>
#include <stdint.h>

struct kernel_data;
class serial_port;

class console
{
public:
    console();
    console(serial_port *serial);

    void set_color(uint8_t fg = 7, uint8_t bg = 0);

    void putc(char c);
    size_t puts(const char *message);
    void vprintf(const char *fmt, va_list args);

    inline void printf(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }

    template <typename T>
    void put_hex(T x, int width = 0, char pad = ' ');

    template <typename T>
    void put_dec(T x, int width = 0, char pad = ' ');

    void echo();

    static console * get();

private:
    serial_port *m_serial;
};

extern console &g_console;
inline console * console::get() { return &g_console; }


extern const char digits[];

template <typename T>
void console::put_hex(T x, int width, char pad)
{
    static const int chars = sizeof(x) * 2;
    char message[chars + 1];

    int len = 1;
    for (int i = chars - 1; i >= 0; --i) {
        uint8_t nibble = (x >> (i*4)) & 0xf;
        if (nibble) len = len > i + 1 ? len : i + 1;
        message[chars - i - 1] = digits[nibble];
    }
    message[chars] = 0;

    if (width > len) for(int i=0; i<(width-len); ++i) putc(pad);
    puts(message + (chars - len));
    if (-width > len) for(int i=0; i<(-width-len); ++i) putc(' ');
}

template <typename T>
void console::put_dec(T x, int width, char pad)
{
    static const int chars = sizeof(x) * 3;
    char message[chars + 1];
    char *p = message + chars;
    int length = 0;
    *p-- = 0;
    do {
        *p-- = digits[x % 10];
        x /= 10;
        length += 1;
    } while (x != 0);

    if (width > length) for(int i=0; i<(width-length); ++i) putc(pad);
    puts(++p);
    if (-width > length) for(int i=0; i<(-width-length); ++i) putc(' ');
}
