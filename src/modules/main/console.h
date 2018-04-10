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

	size_t puts(const char *message);
	size_t printf(const char *fmt, ...);

private:
	char * line_pointer(unsigned line);

	font m_font;
	screen m_screen;

	coord<unsigned> m_size;
	coord<unsigned> m_pos;
	screen::pixel_t m_fg;
	screen::pixel_t m_bg;

	size_t m_first;
	size_t m_length;

	char *m_data;
};
