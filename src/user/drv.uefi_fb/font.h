#pragma once
#include <stdint.h>

#include "screen.h"

class font
{
public:
	/// Constructor.
	/// \arg data  The font data to load. If null, will load the default
	///            built-in font.
	font(void const *data = nullptr);

	unsigned glyph_bytes() const { return m_sizey * ((m_sizex + 7) / 8); }
	unsigned count() const { return m_count; }
	unsigned width() const { return m_sizex; }
	unsigned height() const { return m_sizey; }
	bool valid() const { return m_count > 0; }

	void draw_glyph(
			screen &s,
			uint32_t glyph,
			screen::pixel_t fg,
			screen::pixel_t bg,
			unsigned x,
			unsigned y) const;

private:
	unsigned m_sizex, m_sizey;
	unsigned m_count;
	uint8_t const *m_data;
};

