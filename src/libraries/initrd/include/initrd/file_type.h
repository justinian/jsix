#pragma once
#include <stdint.h>

namespace initrd {

enum class file_type : uint8_t
{
	unknown,
	executable,
	vdso
};

} // namespace initrd
