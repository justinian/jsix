#include "font.h"

/* PSF2 header format
 * Taken from the Linux KBD documentation
 * http://www.win.tue.nl/~aeb/linux/kbd/font-formats-1.html
 */

const static uint8_t magic[] = {0x72, 0xb5, 0x4a, 0x86};
const static uint32_t max_version = 0;

const static uint8_t unicode_sep = 0xff;
const static uint8_t unicode_start = 0xfe;

/* bits used in flags */
enum psf2_flags {
	psf2_has_unicode = 0x00000001
};

struct psf2_header {
	uint8_t magic[4];
	uint32_t version;
	uint32_t header_size;	// offset of bitmaps in file
	uint32_t flags;
	uint32_t length;		// number of glyphs
	uint32_t charsize;		// number of bytes for each character
	uint32_t height, width; // max dimensions of glyphs
};

font
font::load(void const *data)
{
	psf2_header const *psf2 = static_cast<psf2_header const *>(data);
	for (int i = 0; i < sizeof(magic); ++i) {
		if (psf2->magic[i] != magic[i]) {
			return font{};
		}
	}

	uint8_t const *font_data = static_cast<uint8_t const *>(data) + psf2->header_size;
	return font{
			psf2->height,
			psf2->width,
			psf2->length,
			font_data};
}

font::font() :
	m_count(0),
	m_data(nullptr)
{}

font::font(unsigned height, unsigned width, unsigned count, uint8_t const *data) :
	m_size(width, height),
	m_count(count),
	m_data(data)
{}

font::font(const font &other) :
	m_size(other.m_size),
	m_count(other.m_count),
	m_data(other.m_data)
{}

void
font::draw_glyph(
		screen &s,
		uint32_t glyph,
		screen::pixel_t fg,
		screen::pixel_t bg,
		unsigned x,
		unsigned y) const
{
	unsigned bwidth = (m_size.x+7)/8;
	uint8_t const *data = m_data + (glyph * glyph_bytes());

	for (int dy = 0; dy < m_size.y; ++dy) {
		for (int dx = 0; dx < bwidth; ++dx) {
			uint8_t byte = data[dy * bwidth + dx];
			for (int i = 0; i < 8; ++i) {
				if (dx*8 + i >= m_size.x) continue;
				const uint8_t mask = 1 << (7-i);
				uint32_t c = (byte & mask) ? fg : bg;
				s.draw_pixel(x + dx*8 + i, y + dy, c);
			}
		}
	}
}

