#include "kassert.h"

using __exit_func = void (*)(void *);

extern "C" {
    void *__dso_handle __attribute__ ((__weak__));
    int __cxa_atexit(__exit_func, void *, void *);
    void __cxa_pure_virtual();
}


struct __exit_func_entry
{
    __exit_func func;
    void *obj;
    void *dso;
};

static int __num_exit_funcs = 0;
static const int __max_exit_funcs = 64;
__exit_func_entry __exit_funcs[__max_exit_funcs];

int
__cxa_atexit(__exit_func f, void *o, void *dso)
{
    int i = __num_exit_funcs++;
    if (i >= __max_exit_funcs) return -1;
    __exit_funcs[i].func = f;
    __exit_funcs[i].obj = o;
    __exit_funcs[i].dso = dso;
    return 0;
}

void __cxa_pure_virtual()
{
    kassert(0, "Pure virtual function call");
}
