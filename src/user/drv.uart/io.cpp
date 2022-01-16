#include "io.h"

uint8_t
inb(uint16_t port)
{
    uint8_t val;
    __asm__ __volatile__ ( "inb %1, %0" : "=a"(val) : "Nd"(port) );
    return val;
}

void
outb(uint16_t port, uint8_t val)
{
    __asm__ __volatile__ ( "outb %0, %1" :: "a"(val), "Nd"(port) );
}

void
io_wait(unsigned times)
{
    for (unsigned i = 0; i < times; ++i)
        outb(0x80, 0);
}
