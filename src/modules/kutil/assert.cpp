#include "assert.h"

namespace kutil {

assert_callback __kernel_assert_p = nullptr;

assert_callback
assert_set_callback(assert_callback f)
{
	assert_callback old = __kernel_assert_p;
	__kernel_assert_p = f;
	return old;
}


} // namespace kutil
