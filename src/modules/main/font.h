#pragma once
#include <stdint.h>

struct screen;

struct font {
    uint16_t height;
    uint16_t width;
    uint16_t charsize;
    uint16_t count;
    uint8_t *data;
};

int font_init(void *header, struct font *font);

int font_draw(
    struct font *f,
    struct screen *s,
    uint32_t glyph,
    uint32_t color,
    uint32_t x,
    uint32_t y);
