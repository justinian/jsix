#pragma once
/// \file console.h
/// Text output handler

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

namespace uefi {
namespace protos {
    struct simple_text_output;
}}

namespace boot {

/// Object providing basic output functionality to the UEFI console
class console
{
public:
    console(uefi::protos::simple_text_output *out);

    void announce();

    size_t print_hex(uint32_t n) const;
    size_t print_dec(uint32_t n) const;
    size_t print_long_hex(uint64_t n) const;
    size_t print_long_dec(uint64_t n) const;
    size_t printf(const wchar_t *fmt, ...) const;

    static console & get() { return *s_console; }
    static size_t print(const wchar_t *fmt, ...);

private:
    friend class status_line;

    size_t vprintf(const wchar_t *fmt, va_list args) const;

    size_t m_rows, m_cols;
    uefi::protos::simple_text_output *m_out;

    static console *s_console;
};

size_t wstrlen(const wchar_t *s);

} // namespace boot
