#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// kutil malloc/free stubs
#include <malloc.h>
namespace kutil {
	void * malloc(size_t n) { return ::malloc(n); }
	void free(void *p) { ::free(p); }
}
