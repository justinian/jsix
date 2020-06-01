#pragma once
/// \file slab_allocated.h
/// A parent template class for slab-allocated objects

#include "kernel_memory.h"
#include "kutil/memory.h"
#include "kutil/vector.h"

namespace kutil {

template <typename T, unsigned N = 1>
class slab_allocated
{
public:
	void * operator new(size_t size)
	{
		kassert(size == sizeof(T), "Slab allocator got wrong size allocation");
		if (s_free.count() == 0)
			allocate_chunk();

		T *item = s_free.pop();
		kutil::memset(item, 0, sizeof(T));
		return item;
	}

	void operator delete(void *p) { s_free.append(reinterpret_cast<T*>(p)); }

	// TODO: get rid of this terribleness
	static void hacky_init_remove_me() {
		memset(&s_free, 0, sizeof(s_free));
	}

private:
	static void allocate_chunk()
	{
		size_t size = N * ::memory::frame_size;
		s_free.ensure_capacity(size / sizeof(T));

		void *memory = kalloc(size);
		T *current = reinterpret_cast<T *>(memory); 
		T *end = offset_pointer(current, size);
		while (current < end)
			s_free.append(current++);
	}

	static vector<T*> s_free;
};

#define DEFINE_SLAB_ALLOCATOR(type, N) \
	template<> ::kutil::vector<type*> kutil::slab_allocated<type, N>::s_free {};

} // namespace kutil
