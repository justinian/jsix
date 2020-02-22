#include "utility.h"

size_t
wstrlen(const wchar_t *s)
{
	size_t count = 0;
	while (s && *s++) count++;
	return count;
}

