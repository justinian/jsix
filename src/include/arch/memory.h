#pragma once
/// \file arch/memory.h
/// Include architecture-specific constants related to memory and paging

#if defined(__amd64__) || defined(__x86_64__)
#include <arch/amd64/memory.h>
#else
#error Unsupported platform.
#endif
