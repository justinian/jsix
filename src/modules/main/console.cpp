#include "console.h"

console::console(const font &f, const screen &s, void *scratch, size_t len) :
	m_font(f),
	m_screen(s),
	m_size(s.width() / f.width(), s.height() / f.height()),
	m_fg(0xffffffff),
	m_bg(0x00000000),
	m_first(0),
	m_length(len),
	m_data(static_cast<char *>(scratch))
{
	if (len < m_size.size()) {
		m_data = nullptr;
	}

	const unsigned count = m_size.size();
	for (unsigned i = 0; i < count; ++i)
		m_data[i] = 0;
	repaint();
}

char *
console::line_pointer(unsigned line)
{
	if (!m_data) return nullptr;
	return m_data + ((m_first + line) % m_size.y) * m_size.x;
}

void
console::repaint()
{
	m_screen.fill(m_bg);
	if (!m_data) return;

	for (unsigned y = 0; y < m_size.y; ++y) {
		const char *line = line_pointer(y);
		for (unsigned x = 0; x < m_size.x; ++x) {
			m_font.draw_glyph(
				m_screen,
				line[x] ? line[x] : ' ',
				m_fg,
				m_bg,
				x * m_font.width(),
				y * m_font.height());
		}
	}
}

void
console::scroll(unsigned lines)
{
	if (!m_data) {
		m_pos.x = 0;
		m_pos.y = 0;
	} else {
		unsigned bytes = lines * m_size.x;
		char *line = line_pointer(0);
		for (unsigned i = 0; i < bytes; ++i)
			*line++ = 0;

		m_first = (m_first + lines) % m_size.y;
		m_pos.y -= lines;
	}
	repaint();
}

size_t
console::puts(const char *message)
{
	char *line = line_pointer(m_pos.y);

	while (message && *message) {
		const unsigned x = m_pos.x * m_font.width();
		const unsigned y = m_pos.y * m_font.height();
		const char c = *message++;

		switch (c) {
		case '\t':
			m_pos.x = (m_pos.x + 4) / 4 * 4;
			break;

		case '\r':
			m_pos.x = 0;
			break;

		case '\n':
			m_pos.x = 0;
			m_pos.y++;
			break;

		default:
			if (m_data) line[m_pos.x] = c;
			m_font.draw_glyph(m_screen, c, m_fg, m_bg, x, y);
			m_pos.x++;
		}

		if (m_pos.x >= m_size.x) {
			m_pos.x = m_pos.x % m_size.x;
			m_pos.y++;
		}

		if (m_pos.y >= m_size.y) {
			scroll(1);
			line = line_pointer(m_pos.y);
		}
	}
}

