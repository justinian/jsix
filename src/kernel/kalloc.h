#pragma once
/// \file kalloc.h
/// Definitions of kalloc() and kfree()

#include <stddef.h>


/// Allocate kernel space memory.
/// \arg length  The amount of memory to allocate, in bytes
/// \returns     A pointer to the allocated memory, or nullptr if
///              allocation failed.
inline void * kalloc(size_t length);

/// Free kernel space memory.
/// \arg p   The pointer to free
inline void kfree(void *p);

