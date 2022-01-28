#include "assert.h"
#include "idt.h"

namespace panic {

uint32_t *apic_icr = reinterpret_cast<uint32_t*>(0xffffc000fee00300);
void const *symbol_table = nullptr;

void
install(uintptr_t entrypoint, const void *symbol_data)
{
    IDT::set_nmi_handler(entrypoint);
    symbol_table = symbol_data;
}

} // namespace panic

extern "C"
void _PDCLIB_assert(const char *message, const char *function, const char *file, unsigned line) {
    panic::panic(message, nullptr, function, file, line);
}
