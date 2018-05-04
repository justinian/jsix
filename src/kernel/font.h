#pragma once
#include <stdint.h>

#include "kutil/coord.h"
#include "screen.h"

class font
{
public:
	font(void const *data);

	unsigned glyph_bytes() const { return m_size.y * ((m_size.x + 7) / 8); }
	unsigned count() const { return m_count; }
	unsigned width() const { return m_size.x; }
	unsigned height() const { return m_size.y; }
	bool valid() const { return m_count > 0; }

	void draw_glyph(
			screen *s,
			uint32_t glyph,
			screen::pixel_t fg,
			screen::pixel_t bg,
			unsigned x,
			unsigned y) const;

private:
	kutil::coord<unsigned> m_size;
	unsigned m_count;
	uint8_t const *m_data;

	font() = delete;
};

