#pragma once
/// \file j6/syslog.hh
/// Utility function for writing messages to the kernel log

// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

namespace j6 {

void syslog(const char *fmt, ...);

} // namespace j6

#endif // __j6kernel
