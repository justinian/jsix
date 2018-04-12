#include "screen.h"

template <typename T>
static int popcount(T x)
{
	int c = 0;
	while (x) {
		c += (x & 1);
		x = x >> 1;
	}
	return c;
}

template <typename T>
static int ctz(T x)
{
	int c = 0;
	while ((x & 1) == 0) {
		x = x >> 1;
		++c;
	}
	return c;
}

screen::color_masks::color_masks(pixel_t r, pixel_t g, pixel_t b) :
	r(r), g(g), b(b)
{
	rshift = static_cast<uint8_t>(ctz(r) - (8 - popcount(r)));
	gshift = static_cast<uint8_t>(ctz(g) - (8 - popcount(g)));
	bshift = static_cast<uint8_t>(ctz(b) - (8 - popcount(b)));
}

screen::color_masks::color_masks(const color_masks &o) :
	rshift(o.rshift), gshift(o.gshift), bshift(o.bshift),
	r(o.r), g(o.g), b(o.b)
{
}

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

screen::pixel_t
screen::color(uint8_t r, uint8_t g, uint8_t b) const
{
	return
		((static_cast<uint32_t>(r) << m_masks.rshift) & m_masks.r) |
		((static_cast<uint32_t>(g) << m_masks.gshift) & m_masks.g) |
		((static_cast<uint32_t>(b) << m_masks.bshift) & m_masks.b);
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
