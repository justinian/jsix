#include "assert.h"
#include "idt.h"

namespace panic {

uint32_t *apic_icr = reinterpret_cast<uint32_t*>(0xffffc000fee00300);
void const *symbol_table = nullptr;

void
install(uintptr_t entrypoint, util::const_buffer symbol_data)
{
    IDT::set_nmi_handler(entrypoint);
    symbol_table = symbol_data.pointer;
}

} // namespace panic

extern "C"
void __assert_fail(const char *message, const char *file, unsigned line, const char *function) {
    panic::panic(message, nullptr, function, file, line);
}
