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

uint64_t
rdmsr(uint64_t addr)
{
	uint32_t low, high;
	__asm__ __volatile__ ("rdmsr" : "=a"(low), "=d"(high) : "c"(addr));
	return (static_cast<uint64_t>(high) << 32) | low;
}

void
wrmsr(uint64_t addr, uint64_t value)
{
	uint32_t low = value & 0xffffffff;
	uint32_t high = value >> 32;
	__asm__ __volatile__ ("wrmsr" :: "c"(addr), "a"(low), "d"(high));
}

void
io_wait()
{
	outb(0x80, 0);
}
