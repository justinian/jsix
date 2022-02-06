#pragma once

#if __has_include("__stddef_max_align_t.h")
#include "__stddef_max_align_t.h"
#elif __BIGGEST_ALIGNMENT__ == __SIZEOF_LONG_DOUBLE__
typedef long double max_align_t;
#elif __BIGGEST_ALIGNMENT__ == __SIZEOF_DOUBLE__
typedef double max_align_t;
#elif __BIGGEST_ALIGNMENT__ == __SIZEOF_LONG_LONG__
typedef long long max_align_t;
#else
#error Can't figure out size of max_align_t!
#endif

