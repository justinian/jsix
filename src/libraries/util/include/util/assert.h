#pragma once
/// \file assert.h
/// Utility header to include the right assert.h for the environment

#if __has_include("kassert.h")
    #include "kassert.h"
#elif __has_include(<assert.h>)
    #include <assert.h>
#endif

#if !defined(assert)
    #define assert(x) ((void)0)
#endif