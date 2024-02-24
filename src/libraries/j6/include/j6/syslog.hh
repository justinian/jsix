#pragma once
/// \file j6/syslog.hh
/// Utility function for writing messages to the kernel log

// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel
#include <stdint.h>
#include <util/api.h>

namespace j6 {

enum class logs : uint8_t {
#define LOG(name, lvl) name,
#include <j6/tables/log_areas.inc>
#undef LOG
    COUNT
};

enum class log_level : uint8_t {
    silent, fatal, error, warn, info, verbose, spam, max
};

void API syslog(logs area, log_level severity, const char *fmt, ...);

} // namespace j6

#endif // __j6kernel
