#include "msr.h"

uint64_t
rdmsr(msr addr)
{
	uint32_t low, high;
	__asm__ __volatile__ ("rdmsr" : "=a"(low), "=d"(high) : "c"(addr));
	return (static_cast<uint64_t>(high) << 32) | low;
}

void
wrmsr(msr addr, uint64_t value)
{
	uint32_t low = value & 0xffffffff;
	uint32_t high = value >> 32;
	__asm__ __volatile__ ("wrmsr" :: "c"(addr), "a"(low), "d"(high));
}

