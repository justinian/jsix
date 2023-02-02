#include <stddef.h>
#include <stdint.h>

#include <uefi/protos/simple_text_output.h>
#include <uefi/types.h>
#include <util/format.h>

#include "console.h"
#include "error.h"

#ifndef GIT_VERSION_WIDE
#define GIT_VERSION_WIDE L"no version"
#endif

namespace boot {

size_t ROWS = 0;
size_t COLS = 0;

console *console::s_console = nullptr;

static const wchar_t digits[] = {u'0', u'1', u'2', u'3', u'4', u'5',
    u'6', u'7', u'8', u'9', u'a', u'b', u'c', u'd', u'e', u'f'};


size_t
wstrlen(const wchar_t *s)
{
    size_t count = 0;
    while (s && *s++) count++;
    return count;
}


console::console(uefi::protos::simple_text_output *out) :
    m_rows {0},
    m_cols {0},
    m_out {out}
{
    try_or_raise(
        m_out->query_mode(m_out->mode->mode, &m_cols, &m_rows),
        L"Failed to get text output mode.");
    try_or_raise(
        m_out->clear_screen(),
        L"Failed to clear screen");
    s_console = this;
}

void
console::announce()
{
    m_out->set_attribute(uefi::attribute::light_cyan);
    m_out->output_string(L"jsix loader ");

    m_out->set_attribute(uefi::attribute::light_magenta);
    m_out->output_string(GIT_VERSION_WIDE);

    m_out->set_attribute(uefi::attribute::light_gray);
    m_out->output_string(L" booting...\r\n");
}

size_t
console::vprintf(const wchar_t *fmt, va_list args) const
{
    wchar_t buffer[256];
    util::counted<wchar_t> output {buffer, sizeof(buffer)/sizeof(wchar_t)};
    size_t count = util::vformat(output, fmt, args);

    m_out->output_string(buffer);
    return count;
}

size_t
console::printf(const wchar_t *fmt, ...) const
{
    va_list args;
    va_start(args, fmt);

    size_t result = vprintf(fmt, args);

    va_end(args);
    return result;
}

size_t
console::print(const wchar_t *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    size_t result = get().vprintf(fmt, args);

    va_end(args);
    return result;
}

} // namespace boot
