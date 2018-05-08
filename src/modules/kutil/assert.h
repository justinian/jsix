#pragma once

namespace kutil {

using assert_callback =
	[[noreturn]] void (*)
	(const char *file, unsigned line, const char *message);

/// Set the global kernel assert callback
/// \args f   The new callback
/// \returns  The old callback
assert_callback assert_set_callback(assert_callback f);

extern assert_callback __kernel_assert_p;

} // namespace kutil


#define kassert(stmt, message)  if(!(stmt)) { ::kutil::__kernel_assert_p(__FILE__, __LINE__, (message)); } else {}
