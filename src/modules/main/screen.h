#pragma once
#include <stddef.h>
#include <stdint.h>

class screen
{
public:
    using coord_t = uint32_t;
    using pixel_t = uint32_t;

    screen(
        void *framebuffer,
        coord_t hres, coord_t vres,
        pixel_t rmask, pixel_t gmask, pixel_t bmask);

    void fill(pixel_t color);
    void draw_pixel(coord_t x, coord_t y, pixel_t color);

    screen() = delete;
    screen(const screen &) = delete;

    struct color_masks {
        pixel_t r, g, b;
        color_masks(pixel_t r, pixel_t g, pixel_t b);
    };

    struct resolution {
        coord_t w, h;
        resolution(coord_t w, coord_t h);
        coord_t size() const { return w * h; }
    };

private:
    pixel_t *m_framebuffer;
    color_masks m_masks;
    resolution m_resolution;
};

