#pragma once
#include <stddef.h>
#include <stdint.h>

class screen
{
public:
    using pixel_t = uint32_t;

    enum class pixel_order : uint8_t { bgr8, rgb8, };

    screen(volatile void *addr, unsigned hres, unsigned vres, unsigned scanline, pixel_order order);

    unsigned width() const { return m_resx; }
    unsigned height() const { return m_resy; }

    pixel_t color(uint8_t r, uint8_t g, uint8_t b) const;

    void fill(pixel_t color);

    inline void draw_pixel(unsigned x, unsigned y, pixel_t color) {
        const size_t index = x + y * m_scanline;
        m_back[index] = color;
    }

    void update();

private:
    volatile pixel_t *m_fb;
    pixel_t *m_back;
    pixel_order m_order;
    unsigned m_scanline;
    unsigned m_resx, m_resy;

    screen() = delete;
};
