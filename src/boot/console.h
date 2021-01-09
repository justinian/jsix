/// \file console.h
/// Text output handler
#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <uefi/boot_services.h>
#include <uefi/protos/simple_text_output.h>
#include "kernel_args.h"
#include "types.h"

namespace boot {

/// Object providing basic output functionality to the UEFI console
class console
{
public:
	using framebuffer = kernel::args::framebuffer;

	console(uefi::boot_services *bs, uefi::protos::simple_text_output *out);

	size_t print_hex(uint32_t n) const;
	size_t print_dec(uint32_t n) const;
	size_t print_long_hex(uint64_t n) const;
	size_t print_long_dec(uint64_t n) const;
	size_t printf(const wchar_t *fmt, ...) const;

	const framebuffer & fb() const { return m_fb; };

	static console & get() { return *s_console; }
	static size_t print(const wchar_t *fmt, ...);

private:
	friend class status_line;

	void pick_mode(uefi::boot_services *bs);
	size_t vprintf(const wchar_t *fmt, va_list args) const;

	size_t m_rows, m_cols;
	uefi::protos::simple_text_output *m_out;
	framebuffer m_fb;

	static console *s_console;
};

} // namespace boot
