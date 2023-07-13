/// \file format.h
/// sprintf-like format functions

#pragma once

#include <stdarg.h>
#include <util/counted.h>

namespace util {

template <typename char_t>
size_t format(counted<char_t> output, const char_t *format, ...);

template <typename char_t>
size_t vformat(counted<char_t> output, const char_t *format, va_list va);

}
