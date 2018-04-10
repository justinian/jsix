#include "screen.h"

screen::color_masks::color_masks(pixel_t r, pixel_t g, pixel_t b) : r(r), g(g), b(b) {}
screen::color_masks::color_masks(const color_masks &o) : r(o.r), g(o.g), b(o.b) {}

screen::screen(
		void *framebuffer,
		unsigned hres, unsigned vres,
		pixel_t rmask, pixel_t gmask, pixel_t bmask) :
	m_framebuffer(static_cast<pixel_t *>(framebuffer)),
	m_masks(rmask, gmask, bmask),
	m_resolution(hres, vres)
{
}

screen::screen(const screen &other) :
	m_framebuffer(other.m_framebuffer),
	m_masks(other.m_masks),
	m_resolution(other.m_resolution)
{
}

void
screen::fill(pixel_t color)
{
	const size_t len = m_resolution.size();
	for (size_t i = 0; i < len; ++i)
		m_framebuffer[i] = color;
}

void
screen::draw_pixel(unsigned x, unsigned y, pixel_t color)
{
	m_framebuffer[x + y * m_resolution.x] = color;
}
