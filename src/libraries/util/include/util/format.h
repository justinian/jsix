/// \file format.h
/// sprintf-like format functions

#pragma once

#include <stdarg.h>
#include <util/api.h>
#include <util/counted.h>

namespace util {

size_t API format(counted<char> output, const char *format, ...);
size_t API vformat(counted<char> output, const char *format, va_list va);


size_t API format(counted<wchar_t> output, const wchar_t *format, ...);
size_t API vformat(counted<wchar_t> output, const wchar_t *format, va_list va);

}
