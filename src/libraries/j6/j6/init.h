#pragma once
/// \file init.h
/// Process initialization utility functions

#include <j6/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct j6_init_args
{
    uint64_t args[2];
};

/// Find the first handle of the given type held by this process
j6_handle_t j6_find_first_handle(j6_object_type obj_type);

/// Drivers may use driver_main instead of main
int driver_main(unsigned, const char **, const char **, const j6_init_args *);

#ifdef __cplusplus
} // extern "C"
#endif
