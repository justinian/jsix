#include <stdarg.h>
#include "console.h"
#include "io.h"

const char digits[] = "0123456789abcdef";
console g_console;


class console_out_screen
{
public:
	console_out_screen(const font &f, const screen &s, void *scratch, size_t len) :
		m_font(f),
		m_screen(s),
		m_size(s.width() / f.width(), s.height() / f.height()),
		m_fg(0xffffff),
		m_bg(0),
		m_first(0),
		m_length(len),
		m_data(nullptr),
		m_attrs(nullptr),
		m_palette(nullptr)
	{
		const unsigned count = m_size.size();
		const size_t palette_size = sizeof(screen::pixel_t) * 256;
		const size_t attrs_size = 2 * count;

		if (len >= count) {
			// We have enough scratch space to keep the contents of the screen
			m_data = static_cast<char *>(scratch);
			for (unsigned i = 0; i < count; ++i)
				m_data[i] = 0;
		}

		if (len >= count + palette_size + attrs_size) {
			// We have enough scratch space to also keep the colors of the text
			m_palette = reinterpret_cast<screen::pixel_t *>(m_data + count);

			unsigned index = 0;

			// Manually add the 16 basic ANSI colors
			m_palette[index++] = m_screen.color(0x00, 0x00, 0x00);
			m_palette[index++] = m_screen.color(0xcd, 0x00, 0x00);
			m_palette[index++] = m_screen.color(0x00, 0xcd, 0x00);
			m_palette[index++] = m_screen.color(0xcd, 0xcd, 0x00);
			m_palette[index++] = m_screen.color(0x00, 0x00, 0xee);
			m_palette[index++] = m_screen.color(0xcd, 0x00, 0xcd);
			m_palette[index++] = m_screen.color(0x00, 0xcd, 0xcd);
			m_palette[index++] = m_screen.color(0xe5, 0xe5, 0xe5);
			m_palette[index++] = m_screen.color(0x7f, 0x7f, 0x7f);
			m_palette[index++] = m_screen.color(0xff, 0x00, 0x00);
			m_palette[index++] = m_screen.color(0x00, 0xff, 0x00);
			m_palette[index++] = m_screen.color(0xff, 0xff, 0x00);
			m_palette[index++] = m_screen.color(0x00, 0x50, 0xff);
			m_palette[index++] = m_screen.color(0xff, 0x00, 0xff);
			m_palette[index++] = m_screen.color(0x00, 0xff, 0xff);
			m_palette[index++] = m_screen.color(0xff, 0xff, 0xff);

			// Build the high-color portion of the table
			const uint32_t intensity[] = {0, 0x5f, 0x87, 0xaf, 0xd7, 0xff};
			const uint32_t intensities = sizeof(intensity) / sizeof(uint32_t);

			for (uint32_t r = 0; r < intensities; ++r) {
				for (uint32_t g = 0; g < intensities; ++g) {
					for (uint32_t b = 0; b < intensities; ++b) {
						m_palette[index++] = m_screen.color(
								intensity[r], intensity[g], intensity[b]);
					}
				}
			}

			// Build the greyscale portion of the table
			for (uint8_t i = 0x08; i <= 0xee; i += 10)
				m_palette[index++] = m_screen.color(i, i, i);

			set_color(7, 0); // Grey on black default
			m_attrs = reinterpret_cast<uint16_t *>(m_data + count + palette_size);
			for (unsigned i = 0; i < count; ++i)
				m_attrs[i] = m_attr;
		}

		repaint();
	}

	void repaint()
	{
		m_screen.fill(m_bg);
		if (!m_data) return;

		for (unsigned y = 0; y < m_size.y; ++y) {
			const char *line = line_pointer(y);
			const uint16_t *attrs = attr_pointer(y);
			for (unsigned x = 0; x < m_size.x; ++x) {
				const uint16_t attr = attrs[x];

				set_color(static_cast<uint8_t>(attr),
						static_cast<uint8_t>(attr >> 8));

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
		if (!m_palette) return;

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

				const unsigned x = m_pos.x * m_font.width();
				const unsigned y = m_pos.y * m_font.height();
				m_font.draw_glyph(m_screen, c, m_fg, m_bg, x, y);

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

	font m_font;
	screen m_screen;

	kutil::coord<unsigned> m_size;
	kutil::coord<unsigned> m_pos;
	screen::pixel_t m_fg, m_bg;
	uint16_t m_attr;

	size_t m_first;
	size_t m_length;

	char *m_data;
	uint16_t *m_attrs;
	screen::pixel_t *m_palette;
};

console_out_screen *
console_get_screen_out(const font &f, const screen &s, void *scratch, size_t len)
{
	return nullptr;
	//return new console_out_screen(f, s, scratch, len);
}


static bool
serial_ready()
{
	return (inb(COM1 + 5) & 0x20) != 0;
}

static void
serial_write(char c) {
	while (!serial_ready());
	outb(COM1, c);
}


console::console() :
	m_screen(nullptr)
{
}

void
console::set_color(uint8_t fg, uint8_t bg)
{
	if (m_screen)
		m_screen->set_color(fg, bg);
}

void
console::puts(const char *message)
{
	while (message && *message) {
		char c = *message++;
		if (m_screen) m_screen->putc(c);

		serial_write(c);
		if (c == '\n') serial_write('\r');
	}
}

void console::printf(const char *fmt, ...)
{
	static const unsigned buf_size = 256;
	char buffer[256];

	const char *r = fmt;
	char *w = buffer;
	char *wend = buffer + buf_size;

	va_list args;
	va_start(args, fmt);

#define flush() do { *w = 0; puts(buffer); w = buffer; } while(0)

	while (r && *r) {
		if (w == wend) flush();

		if (*r != '%') {
			*w++ = *r++;
			continue;
		}

		r++; // chomp the %
		flush();

		bool done = false;
		bool right = true;
		int width = 0;

		while (!done) {
			char c = *r++;
			switch (c) {
				case '%': *w = '%'; done = true; break;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9': width = width * 10 + (c - '0'); break;

				case '-': right = !right; break;

				case 'x': put_hex<uint32_t>(va_arg(args, uint32_t), right ? width : -width); done = true; break;

				case 'd':
				case 'u': put_dec<uint32_t>(va_arg(args, uint32_t), right ? width : -width); done = true; break;

				case 's': {
						const char *s = va_arg(args, const char*);
						if (s) puts(s);
					}
					done = true;
					break;

				case 'l':
					switch (*r++) {
						case 'x': put_hex<uint64_t>(va_arg(args, uint64_t), right ? width : -width); done = true; break;

						case 'd':
						case 'u': put_dec<uint32_t>(va_arg(args, uint64_t), right ? width : -width); done = true; break;

						default:
							done = true;
							break;
					}
					break;

				default:
					done = true;
					break;
			}
		}
	}

	flush();
	va_end(args);
}
