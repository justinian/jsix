#include <stdlib.h>
#include <j6/syscalls.h>

using cxa_callback = void (*)(void *);
using c_callback = void (*)(void);

namespace {

constexpr size_t max_atexit = 64;

struct atexit_item
{
    cxa_callback cb;
    void *arg;
};

size_t atexit_count = 0;
atexit_item atexit_array[max_atexit];

size_t quick_exit_count = 0;
atexit_item quick_exit_array[max_atexit];

[[noreturn]] inline void
exit_with_callbacks(long status, atexit_item *cbs, size_t count)
{
    for (size_t i = count - 1; i < count; --i) {
        atexit_item &item = cbs[i];
        item.cb(item.arg);
    }

    _Exit(status);
}

void
bare_callback(void *real_cb)
{
    reinterpret_cast<c_callback>(real_cb)();
}

} // namespace

extern "C" int
__cxa_atexit(cxa_callback cb, void *arg, void *dso)
{
    if (atexit_count == max_atexit) return -1;
    atexit_array[atexit_count++] = {cb, arg};
    return 0;
}

int
atexit( void (*func)(void) )
{
    return __cxa_atexit(bare_callback, reinterpret_cast<void*>(func), nullptr);
}

int at_quick_exit( void (*func)(void) )
{
    if (quick_exit_count == max_atexit) return -1;
    quick_exit_array[quick_exit_count++] = {&bare_callback, reinterpret_cast<void*>(func)};
    return 0;
}

void
exit(long status)
{
    exit_with_callbacks(status, atexit_array, atexit_count);
}

void
quick_exit(long status)
{
    exit_with_callbacks(status, quick_exit_array, quick_exit_count);
}

void
abort()
{
    _Exit(INT64_MIN);
}

void
_Exit( long status )
{
    j6_process_exit(status);
}
