#include <stddef.h>

using cb = void (*)(void);
extern cb __init_array_start[0];
extern cb __init_array_end;

namespace {

void
run_global_ctors()
{
    size_t i = 0;
    while (true) {
        const cb &fp = __init_array_start[i++];
        if (&fp == &__init_array_end)
            return;
        fp();
    }
}

} // namespace

extern "C"
void __init_libc()
{
    run_global_ctors();
}
