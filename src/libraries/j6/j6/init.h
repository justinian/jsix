#pragma once
/// \file init.h
/// Process initialization utility functions

#include <j6/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Find the first handle of the given type held by this process
j6_handle_t j6_find_first_handle(j6_object_type obj_type);

#ifdef __cplusplus
} // extern "C"
#endif
