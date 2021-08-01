#include "kutil/assert.h"

namespace kutil {
namespace assert {

uint32_t *apic_icr = reinterpret_cast<uint32_t*>(0xffffc000fee00300);
uintptr_t symbol_table = 0;

} // namespace assert
} // namespace kutil
