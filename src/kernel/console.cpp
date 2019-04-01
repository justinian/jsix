#include "kutil/coord.h"
#include "kutil/guid.h"
#include "kutil/memory.h"
#include "kutil/printf.h"
#include "console.h"
#include "font.h"
#include "screen.h"
#include "serial.h"


const char digits[] = "0123456789abcdef";
console g_console;


class console::screen_out
{
public:
	screen_out(screen *s, font *f) :
		m_font(f),
		m_screen(s),
		m_size(s->width() / f->width(), s->height() / f->height()),
		m_fg(0xffffff),
		m_bg(0),
		m_first(0),
		m_data(nullptr),
		m_attrs(nullptr),
		m_palette(nullptr)
	{
		const unsigned count = m_size.size();
		const size_t attrs_size = 2 * count;

		m_data = new char[count];
		kutil::memset(m_data, 0, count);

		m_palette = new screen::pixel_t[256];
		fill_palette();

		m_attrs = new uint16_t[count];
		set_color(7, 0); // Grey on black default
		for (unsigned i = 0; i < count; ++i) m_attrs[i] = m_attr;

		repaint();
	}

	~screen_out()
	{
		delete [] m_data;
		delete [] m_palette;
		delete [] m_attrs;
	}

	void fill_palette()
	{
		unsigned index = 0;

		// Manually add the 16 basic ANSI colors
		m_palette[index++] = m_screen->color(0x00, 0x00, 0x00);
		m_palette[index++] = m_screen->color(0xcd, 0x00, 0x00);
		m_palette[index++] = m_screen->color(0x00, 0xcd, 0x00);
		m_palette[index++] = m_screen->color(0xcd, 0xcd, 0x00);
		m_palette[index++] = m_screen->color(0x00, 0x00, 0xee);
		m_palette[index++] = m_screen->color(0xcd, 0x00, 0xcd);
		m_palette[index++] = m_screen->color(0x00, 0xcd, 0xcd);
		m_palette[index++] = m_screen->color(0xe5, 0xe5, 0xe5);
		m_palette[index++] = m_screen->color(0x7f, 0x7f, 0x7f);
		m_palette[index++] = m_screen->color(0xff, 0x00, 0x00);
		m_palette[index++] = m_screen->color(0x00, 0xff, 0x00);
		m_palette[index++] = m_screen->color(0xff, 0xff, 0x00);
		m_palette[index++] = m_screen->color(0x00, 0x50, 0xff);
		m_palette[index++] = m_screen->color(0xff, 0x00, 0xff);
		m_palette[index++] = m_screen->color(0x00, 0xff, 0xff);
		m_palette[index++] = m_screen->color(0xff, 0xff, 0xff);

		// Build the high-color portion of the table
		const uint32_t intensity[] = {0, 0x5f, 0x87, 0xaf, 0xd7, 0xff};
		const uint32_t intensities = sizeof(intensity) / sizeof(uint32_t);

		for (uint32_t r = 0; r < intensities; ++r) {
			for (uint32_t g = 0; g < intensities; ++g) {
				for (uint32_t b = 0; b < intensities; ++b) {
					m_palette[index++] = m_screen->color(
							intensity[r], intensity[g], intensity[b]);
				}
			}
		}

		// Build the greyscale portion of the table
		for (uint8_t i = 0x08; i <= 0xee; i += 10)
			m_palette[index++] = m_screen->color(i, i, i);
	}

	void repaint()
	{
		m_screen->fill(m_bg);
		if (!m_data) return;

		for (unsigned y = 0; y < m_size.y; ++y) {
			const char *line = line_pointer(y);
			const uint16_t *attrs = attr_pointer(y);
			for (unsigned x = 0; x < m_size.x; ++x) {
				const uint16_t attr = attrs[x];

				set_color(static_cast<uint8_t>(attr),
						static_cast<uint8_t>(attr >> 8));

				m_font->draw_glyph(
					m_screen,
					line[x] ? line[x] : ' ',
					m_fg,
					m_bg,
					x * m_font->width(),
					y * m_font->height());
			}
		}
	}

	void scroll(unsigned lines)
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

	void set_color(uint8_t fg, uint8_t bg)
	{
		m_bg = m_palette[bg];
		m_fg = m_palette[fg];
		m_attr = (bg << 8) | fg;
	}

	void putc(char c)
	{
		char *line = line_pointer(m_pos.y);
		uint16_t *attrs = attr_pointer(m_pos.y);


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

		default: {
				if (line) line[m_pos.x] = c;
				if (attrs) attrs[m_pos.x] = m_attr;

				const unsigned x = m_pos.x * m_font->width();
				const unsigned y = m_pos.y * m_font->height();
				m_font->draw_glyph(m_screen, c, m_fg, m_bg, x, y);

				m_pos.x++;
			}
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

private:
	char * line_pointer(unsigned line)
	{
		if (!m_data) return nullptr;
		return m_data + ((m_first + line) % m_size.y) * m_size.x;
	}

	uint16_t * attr_pointer(unsigned line)
	{
		if (!m_attrs) return nullptr;
		return m_attrs + ((m_first + line) % m_size.y) * m_size.x;
	}

	font *m_font;
	screen *m_screen;

	kutil::coord<unsigned> m_size;
	kutil::coord<unsigned> m_pos;
	screen::pixel_t m_fg, m_bg;
	uint16_t m_attr;

	size_t m_first;

	char *m_data;
	uint16_t *m_attrs;
	screen::pixel_t *m_palette;
};


console::console() :
	m_screen(nullptr),
	m_serial(nullptr)
{
}

console::console(serial_port *serial) :
	m_screen(nullptr),
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
	if (m_screen)
		m_screen->set_color(fg, bg);

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

void
console::puts(const char *message)
{
	while (message && *message)
		putc(*message++);
}

void
console::putc(char c)
{
	if (m_screen) m_screen->putc(c);

	if (m_serial) {
		m_serial->write(c);
		if (c == '\r') m_serial->write('\n');
	}
}

void console::vprintf(const char *fmt, va_list args)
{
	static const unsigned buf_size = 256;
	char buffer[buf_size];
	vsnprintf_(buffer, buf_size, fmt, args);
	puts(buffer);
}

void
console::init_screen(screen *s, font *f)
{
	m_screen = new screen_out(s, f);
}
