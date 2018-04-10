#include "screen.h"

screen::color_masks::color_masks(pixel_t r, pixel_t g, pixel_t b) : r(r), g(g), b(b) {}
screen::resolution::resolution(coord_t w, coord_t h) : w(w), h(h) {}

screen::screen(
		void *framebuffer,
		coord_t hres, coord_t vres, 
		pixel_t rmask, pixel_t gmask, pixel_t bmask) :
	m_framebuffer(static_cast<pixel_t *>(framebuffer)),
	m_masks(rmask, gmask, bmask),
	m_resolution(hres, vres)
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
screen::draw_pixel(coord_t x, coord_t y, pixel_t color)
{
	m_framebuffer[x + y * m_resolution.w] = color;
}
