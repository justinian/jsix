#pragma once
#include <stdarg.h>
#include <stdint.h>

class font;
struct kernel_data;
class screen;
class serial_port;

class console
{
public:
	console();
	console(serial_port *serial);

	void set_color(uint8_t fg = 7, uint8_t bg = 0);

	void putc(char c);
	void puts(const char *message);
	void vprintf(const char *fmt, va_list args);

	inline void printf(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}

	template <typename T>
	void put_hex(T x, int width = 0);

	template <typename T>
	void put_dec(T x, int width = 0);

	void echo();

	void init_screen(screen *s, font *f);

	static console * get();

private:
	class screen_out;
	screen_out *m_screen;
	serial_port *m_serial;
};

extern console g_console;
inline console * console::get() { return &g_console; }


extern const char digits[];

template <typename T>
void console::put_hex(T x, int width)
{
	static const int chars = sizeof(x) * 2;
	char message[chars + 1];
	for (int i=0; i<chars; ++i) {
		message[chars - i - 1] = digits[(x >> (i*4)) & 0xf];
	}
	message[chars] = 0;

	if (width > chars) for(int i=0; i<(width-chars); ++i) puts(" ");
	puts(message);
	if (-width > chars) for(int i=0; i<(-width-chars); ++i) puts(" ");
}

template <typename T>
void console::put_dec(T x, int width)
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

	if (width > length) for(int i=0; i<(width-length); ++i) puts(" ");
	puts(++p);
	if (-width > length) for(int i=0; i<(-width-length); ++i) puts(" ");
}
