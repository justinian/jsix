/// \file pointer_manipulation.h
/// Helper functions and types for doing type-safe byte-wise pointer math.
#pragma once

namespace boot {

/// Return a pointer offset from `input` by `offset` bytes.
/// \tparam T  Cast the return value to a pointer to `T`
/// \tparam S  The type pointed to by the `input` pointer
template <typename T, typename S>
inline T* offset_ptr(S* input, ptrdiff_t offset) {
	return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(input) + offset);
}

/// Iterator for an array of `T` whose size is known at runtime
/// \tparam T  Type of the objects in the array, whose size might not be
///            what is returned by sizeof(T).
template <typename T>
class offset_iterator
{
public:
	/// Constructor.
	/// \arg t    Pointer to the first item in the array
	/// \arg off  Offset applied to reach successive items. Default is 0,
	///           which creates an effectively constant iterator.
	offset_iterator(T* t, size_t off=0) : m_t(t), m_off(off) {}

	T* operator++() { m_t = offset_ptr<T>(m_t, m_off); return m_t; }
	T* operator++(int) { T* tmp = m_t; operator++(); return tmp; }
	bool operator==(T* p) { return p == m_t; }

	T* operator*() const { return m_t; }
	operator T*() const { return m_t; }
	T* operator->() const { return m_t; }

private:
	T* m_t;
	size_t m_off;
};

} // namespace boot
