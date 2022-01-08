#include "assert.h"

namespace panic {

uint32_t *apic_icr = reinterpret_cast<uint32_t*>(0xffffc000fee00300);
void const *symbol_table = nullptr;

} // namespace panic

extern "C"
void _PDCLIB_assert(const char *message, const char *function, const char *file, unsigned line) {
    panic::panic(message, function, file, line);
}
