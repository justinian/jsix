#pragma once
#include <stdint.h>

/* PSF2 header format
 * Taken from the Linux KBD documentation
 * http://www.win.tue.nl/~aeb/linux/kbd/font-formats-1.html
 */

#define PSF2_MAGIC0     0x72
#define PSF2_MAGIC1     0xb5
#define PSF2_MAGIC2     0x4a
#define PSF2_MAGIC3     0x86

/* bits used in flags */
#define PSF2_HAS_UNICODE_TABLE 0x01

/* max version recognized so far */
#define PSF2_MAXVERSION 0

/* UTF8 separators */
#define PSF2_SEPARATOR  0xFF
#define PSF2_STARTSEQ   0xFE

struct psf2_header {
	uint8_t magic[4];
	uint32_t version;
	uint32_t headersize;    // offset of bitmaps in file
	uint32_t flags;
	uint32_t length;        // number of glyphs
	uint32_t charsize;      // number of bytes for each character
	uint32_t height, width; // max dimensions of glyphs
};
