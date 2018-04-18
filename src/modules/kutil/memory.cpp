#include "memory.h"

namespace kutil {

void *
memset(void *s, uint8_t v, size_t n)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(s);
	for (int i = 0; i < n; ++i) p[i] = 0;
	return s;
}

} // namespace kutil
