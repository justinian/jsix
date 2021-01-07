#include <stdlib.h>
#include <string.h>

#include "font.h"
#include "screen.h"
#include "scrollback.h"

scrollback::scrollback(unsigned lines, unsigned cols, unsigned margin) :
	m_rows {lines},
	m_cols {cols},
	m_count {0},
	m_margin {margin}
{
	m_data = reinterpret_cast<char*>(malloc(lines*cols));
	m_lines = reinterpret_cast<char**>(malloc(lines*sizeof(char*)));
	for (unsigned i = 0; i < lines; ++i)
		m_lines[i] = &m_data[i*cols];

	memset(m_data, ' ', lines*cols);
}

void
scrollback::add_line(const char *line, size_t len)
{
	unsigned i = m_count++ % m_rows;

	if (len > m_cols)
		len = m_cols;

	memcpy(m_lines[i], line, len);
	if (len < m_cols)
		memset(m_lines[i]+len, ' ', m_cols - len);
}

char *
scrollback::get_line(unsigned i)
{
	return m_lines[(i+m_count)%m_rows];
}

void
scrollback::render(screen &scr, font &fnt)
{
	screen::pixel_t fg = scr.color(0xb0, 0xb0, 0xb0);
	screen::pixel_t bg = scr.color(49, 79, 128);

	const unsigned xstride = (m_margin + fnt.width());
	const unsigned ystride = (m_margin + fnt.height());

	for (unsigned y = 0; y < m_rows; ++y) {
		char *line = get_line(y);
		for (unsigned x = 0; x < m_cols; ++x) {
			fnt.draw_glyph(scr, line[x], fg, bg, m_margin+x*xstride, m_margin+y*ystride);
		}
	}
}
