#include <stddef.h>

using cb = void (*)(void);
extern cb __preinit_array_start;
extern cb __preinit_array_end;
extern cb __init_array_start;
extern cb __init_array_end;

namespace {

void
run_ctor_list(cb *p, cb *end)
{
    while (p && end && p < end) {
        if (p) (*p)();
        ++p;
    }
}

void
run_global_ctors()
{
    run_ctor_list(&__preinit_array_start, &__preinit_array_end);
    run_ctor_list(&__init_array_start, &__init_array_end);
}

} // namespace

extern "C" void
__init_libc()
{
    run_global_ctors();
}
