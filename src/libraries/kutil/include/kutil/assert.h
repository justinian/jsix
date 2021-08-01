#pragma once

#include <stdint.h>

namespace kutil {
namespace assert {

constexpr uint32_t send_nmi_command =
    (4 << 8) |  // Delivery mode NMI
    (1 << 14) | // assert level high
    (1 << 18);  // destination self

extern uint32_t *apic_icr;
extern uintptr_t symbol_table;

} // namespace assert
} // namespace kutil

#define kassert(stmt, message)  \
    do { \
        if(!(stmt)) { \
            register const char *m asm("rdi"); \
            register uintptr_t i asm("rsi"); \
            asm volatile ("mov %1, %0" : "=r"(m) : "r"(message)); \
            asm volatile ("mov %1, %0" : "=r"(i) : "r"(kutil::assert::symbol_table)); \
            *kutil::assert::apic_icr = kutil::assert::send_nmi_command; \
            while (1) __asm__ __volatile__ ("hlt"); \
        } \
    } while(0);
