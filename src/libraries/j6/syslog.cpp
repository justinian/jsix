// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <util/format.h>
#include <j6/syscalls.h>
#include <j6/syslog.hh>

namespace j6 {

void
syslog(const char *fmt, ...)
{
    char buffer[200];

    va_list va;
    va_start(va, fmt);
    size_t n = util::vformat({buffer, sizeof(buffer) - 1}, fmt, va);
    va_end(va);

    buffer[n] = 0;
    j6_log(buffer);
}

} // namespace j6

#endif // __j6kernel
