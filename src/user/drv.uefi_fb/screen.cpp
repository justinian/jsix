#include <stdlib.h>
#include <string.h>
#include "screen.h"

screen::screen(volatile void *addr, unsigned hres, unsigned vres, unsigned scanline, pixel_order order) :
    m_fb(static_cast<volatile pixel_t *>(addr)),
    m_order(order),
    m_scanline(scanline),
    m_resx(hres),
    m_resy(vres)
{
    const size_t size = scanline * vres;
    m_back = reinterpret_cast<pixel_t*>(malloc(size * sizeof(pixel_t)));
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
    const size_t len = m_scanline * m_resy;
    asm volatile ( "rep stosl" : :
        "a"(color), "c"(len), "D"(m_back) );
}

void
screen::update()
{
    const size_t len = m_scanline * m_resy * sizeof(pixel_t);
    asm volatile ( "rep movsb" : :
        "c"(len), "S"(m_back), "D"(m_fb) );
}
