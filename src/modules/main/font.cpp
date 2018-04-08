#include "font.h"
#include "screen.h"

/* PSF2 header format
 * Taken from the Linux KBD documentation
 * http://www.win.tue.nl/~aeb/linux/kbd/font-formats-1.html
 */

const static uint8_t magic[] = {0x72, 0xb5, 0x4a, 0x86};

/* bits used in flags */
enum psf2_flags {
	psf2_has_unicode = 0x00000001
};

/* max version recognized so far */
#define PSF2_MAXVERSION 0

/* UTF8 separators */
#define PSF2_SEPARATOR  0xFF
#define PSF2_STARTSEQ   0xFE

struct psf2_header {
	uint8_t magic[4];
	uint32_t version;
	uint32_t header_size;   // offset of bitmaps in file
	uint32_t flags;
	uint32_t length;        // number of glyphs
	uint32_t charsize;      // number of bytes for each character
	uint32_t height, width; // max dimensions of glyphs
};

int
font_init(void *header, struct font *font)
{
	struct psf2_header *psf2 = (struct psf2_header *)header;
	for (int i = 0; i < sizeof(magic); ++i) {
		if (psf2->magic[i] != magic[i]) {
			return 1;
		}
	}

	if (font == 0)
		return 2;

	font->height = psf2->height;
	font->width = psf2->width;
	font->charsize = psf2->charsize;
	font->count = psf2->length;
	font->data = (uint8_t *)header + psf2->header_size;
	return 0;
}

int
font_draw(
	struct font *f,
    struct screen *s,
    uint32_t glyph,
	uint32_t color,
    uint32_t x,
    uint32_t y)
{
	const uint32_t height = f->height;
	const uint32_t width = f->width;
	const uint32_t bwidth = (width+7)/8;
	uint8_t *data = f->data + (glyph * f->charsize);

	for (int dy = 0; dy < f->height; ++dy) {
		for (int dx = 0; dx < bwidth; ++dx) {
			uint8_t byte = data[dy * bwidth + dx];
			for (int i = 0; i < 8; ++i) {
				if (dx*8 + i >= width) continue;
				const uint8_t mask = 1 << (7-i);
				uint32_t c = (byte & mask) ? color : 0;
				screen_pixel(s, x + dx*8 + i, y + dy, c);
			}
		}
	}
	return 0;
}

