#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <malloc.h>
void * operator new (size_t n)					{ return ::malloc(n); }
void * operator new[] (size_t n)				{ return ::malloc(n); }
void operator delete (void *p) noexcept			{ return ::free(p); }
void operator delete[] (void *p) noexcept		{ return ::free(p); }

