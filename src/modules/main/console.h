#pragma once

#include "font.h"
#include "screen.h"
#include "util.h"

struct console_data;

class console
{
public:
	console(const font &f, const screen &s, void *scratch, size_t len);

	void repaint();
	void scroll(unsigned lines);

	void set_color(uint8_t fg, uint8_t bg);

	size_t puts(const char *message);
	size_t printf(const char *fmt, ...);

	template <typename T>
	void put_hex(T x);

	void put_dec(uint32_t x);

private:
	char * line_pointer(unsigned line);
	uint16_t * attr_pointer(unsigned line);

	font m_font;
	screen m_screen;

	coord<unsigned> m_size;
	coord<unsigned> m_pos;
	screen::pixel_t m_fg, m_bg;
	uint16_t m_attr;

	size_t m_first;
	size_t m_length;

	char *m_data;
	uint16_t *m_attrs;
	screen::pixel_t *m_palette;
};

extern const char digits[];

template <typename T>
void console::put_hex(T x)
{
	static const int chars = sizeof(x) * 2;
	char message[chars + 1];
	for (int i=0; i<chars; ++i) {
		message[chars - i - 1] = digits[(x >> (i*4)) & 0xf];
	}
	message[chars] = 0;
	puts(message);
}
