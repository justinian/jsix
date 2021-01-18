#include <stdlib.h>
#include <string.h>
#include "screen.h"

screen::screen(void *addr, unsigned hres, unsigned vres, pixel_order order) :
	m_fb(static_cast<pixel_t *>(addr)),
	m_order(order),
	m_resx(hres),
	m_resy(vres)
{
	m_back = reinterpret_cast<pixel_t*>(malloc(hres*vres*sizeof(pixel_t)));
}

screen::pixel_t
screen::color(uint8_t r, uint8_t g, uint8_t b) const
{
	switch (m_order) {
	case pixel_order::bgr8:
		return
			(static_cast<uint32_t>(b) <<  0) |
			(static_cast<uint32_t>(g) <<  8) |
			(static_cast<uint32_t>(r) << 16);

	case pixel_order::rgb8:
		return
			(static_cast<uint32_t>(r) <<  0) |
			(static_cast<uint32_t>(g) <<  8) |
			(static_cast<uint32_t>(b) << 16);
	}
}

void
screen::fill(pixel_t color)
{
	const size_t len = m_resx * m_resy;
	for (size_t i = 0; i < len; ++i)
		m_back[i] = color;
}

void
screen::draw_pixel(unsigned x, unsigned y, pixel_t color)
{
	m_back[x + y * m_resx] = color;
}

void
screen::update()
{
	memcpy(m_fb, m_back, m_resx*m_resy*sizeof(pixel_t));
}
