#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <malloc.h>
void * operator new (size_t n)					{ return ::malloc(n); }
void * operator new[] (size_t n)				{ return ::malloc(n); }
void operator delete (void *p) noexcept			{ return ::free(p); }
void operator delete[] (void *p) noexcept		{ return ::free(p); }

#include "kutil/heap_manager.h"
#include "kutil/memory.h"

struct default_heap_listener : 
	public Catch::TestEventListenerBase
{
	using TestEventListenerBase::TestEventListenerBase;

	virtual void testCaseStarting(Catch::TestCaseInfo const& info) override
	{
		heap = new kutil::heap_manager(heap_grow_callback);
		kutil::setup::set_heap(heap);
	}

	virtual void testCaseEnded(Catch::TestCaseStats const& stats) override
	{
		kutil::setup::set_heap(nullptr);
		delete heap;

		for (void *p : memory) ::free(p);
		memory.clear();
	}

	static std::vector<void *> memory;
	static kutil::heap_manager *heap;

	static void * heap_grow_callback(size_t length) {
		void *p = aligned_alloc(length, length);
		memory.push_back(p);
		return p;
	}
};

std::vector<void *> default_heap_listener::memory;
kutil::heap_manager *default_heap_listener::heap;

CATCH_REGISTER_LISTENER( default_heap_listener );
