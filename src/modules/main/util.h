#pragma once

template <typename T>
struct coord {
	T x, y;
	coord() : x(T{}), y(T{}) {}
	coord(T x, T y) : x(x), y(y) {}
	T size() const { return x * y; }
};

[[noreturn]] void __kernel_assert(const char *file, unsigned line, const char *message);

#define kassert(stmt, message)  if(!(stmt)) { __kernel_assert(__FILE__, __LINE__, (message)); } else {}

constexpr uint32_t byteswap(uint32_t x)
{
	return ((x >> 24) & 0x000000ff) | ((x >>  8) & 0x0000ff00)
		| ((x <<  8) & 0x00ff0000) | ((x << 24) & 0xff000000);
}
