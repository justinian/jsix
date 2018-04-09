#pragma once
#include <stdint.h>

#include "screen.h"

class font
{
public:
    static font load(void const *data);

    unsigned glyph_bytes() const { return m_height * ((m_width + 7) / 8); }
    unsigned count() const { return m_count; }
    unsigned width() const { return m_width; }
    unsigned height() const { return m_height; }
    bool valid() const { return m_count > 0; }

    void draw_glyph(
            screen &s,
            uint32_t glyph,
            screen::pixel_t color, 
            screen::coord_t x,
            screen::coord_t y) const;

private:
    font();
    font(unsigned height, unsigned width, unsigned count, uint8_t const *data);

    unsigned m_height;
    unsigned m_width;
    unsigned m_count;
    uint8_t const *m_data;
};

