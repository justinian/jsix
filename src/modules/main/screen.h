#pragma once
#include <stddef.h>
#include <stdint.h>

struct screen {
	uint32_t *data;
	uint32_t hres;
	uint32_t vres;
	uint32_t rmask;
	uint32_t gmask;
	uint32_t bmask;
};

int screen_init(
	void *frame_buffer,
	uint32_t hres,
	uint32_t vres,
	uint32_t rmask,
	uint32_t gmask,
	uint32_t bmask,
	struct screen *s);

int screen_fill(struct screen *s, uint32_t color);

int screen_pixel(
	struct screen *s,
	uint32_t x,
	uint32_t y,
	uint32_t color);

