#pragma once

template <typename T>
struct coord {
	T x, y;
	coord() : x(T{}), y(T{}) {}
	coord(T x, T y) : x(x), y(y) {}
	T size() const { return x * y; }
};
