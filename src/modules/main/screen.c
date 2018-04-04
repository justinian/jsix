#include "font.h"
#include "screen.h"

int
screen_fill(struct screen *s, uint32_t color)
{
	const size_t len = s->hres * s->vres;
	for (size_t i = 0; i < len; ++i)
		s->data[i] = 0;
	return 0;
}

int
screen_init(
	void *frame_buffer,
	uint32_t hres,
	uint32_t vres,
	uint32_t rmask,
	uint32_t gmask,
	uint32_t bmask,
	struct screen *s)
{
	s->data = frame_buffer;
	s->hres = hres;
	s->vres = vres;
	s->rmask = rmask;
	s->gmask = gmask;
	s->bmask = bmask;

	return screen_fill(s, 0);
}

int
screen_pixel(
	struct screen *s,
	uint32_t x,
	uint32_t y,
	uint32_t color)
{
	s->data[x + y*s->hres] = color;
	return 0;
}
