/// \file format.h
/// sprintf-like format functions

#pragma once

#include <stdarg.h>
#include <util/counted.h>

namespace util {

using stringbuf = counted<char>;

size_t format(stringbuf output, const char *format, ...);
size_t vformat(stringbuf output, const char *format, va_list va);

}
