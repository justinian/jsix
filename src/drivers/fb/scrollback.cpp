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
	memset(m_data, ' ', lines*cols);
}

void
scrollback::add_line(const char *line, size_t len)
{
	unsigned i = m_count++ % m_rows;

	if (len > m_cols)
		len = m_cols;

	char *start = m_data + (i * m_cols);
	memcpy(start, line, len);
	if (len < m_cols)
		memset(start + len, ' ', m_cols - len);
}

char *
scrollback::get_line(unsigned i)
{
	unsigned line = (i + m_count) % m_rows;
	return &m_data[line*m_cols];
}

void
scrollback::render(screen &scr, font &fnt)
{
	screen::pixel_t fg = scr.color(0xb0, 0xb0, 0xb0);
	screen::pixel_t bg = scr.color(49, 79, 128);

	const unsigned xstride = (m_margin + fnt.width());
	const unsigned ystride = (m_margin + fnt.height());

	for (unsigned y = 0; y < m_rows; ++y) {
		char *line = &m_data[y*m_cols];
		for (unsigned x = 0; x < m_cols; ++x) {
			fnt.draw_glyph(scr, line[x], fg, bg, m_margin+x*xstride, m_margin+y*ystride);
		}
	}
}
