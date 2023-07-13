#pragma once
/// \file new.h
/// Declarations for `operator new` to avoid including <new>

#include <stddef.h>

void *operator new       (size_t);
void *operator new []    (size_t);
void *operator new       (size_t, void *) noexcept;
void *operator new []    (size_t, void *) noexcept;
void  operator delete    (void *) noexcept;
void  operator delete [] (void *) noexcept;
void  operator delete    (void *, void *) noexcept;
void  operator delete [] (void *, void *) noexcept;