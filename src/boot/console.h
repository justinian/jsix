#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <uefi/tables.h>

namespace boot {

class console
{
public:
	console(uefi::system_table *system_table);

	uefi::status initialize(const wchar_t *version);

	void status_begin(const wchar_t *message);
	void status_fail(const wchar_t *error) const;
	void status_warn(const wchar_t *error) const;
	void status_ok() const;

	size_t print_hex(uint32_t n) const;
	size_t print_dec(uint32_t n) const;
	size_t print_long_hex(uint64_t n) const;
	size_t print_long_dec(uint64_t n) const;
	size_t printf(const wchar_t *fmt, ...) const;

	static const console & get() { return *s_console; }
	static size_t print(const wchar_t *fmt, ...);

private:
	uefi::status pick_mode();
	size_t vprintf(const wchar_t *fmt, va_list args) const;

	size_t m_rows, m_cols;
	int m_current_status_line;
	uefi::boot_services *m_boot;
	uefi::protos::simple_text_output *m_out;

	static console *s_console;
};

uefi::status
con_get_framebuffer(
	uefi::boot_services *bs,
	void **buffer,
	size_t *buffer_size,
	uint32_t *hres,
	uint32_t *vres,
	uint32_t *rmask,
	uint32_t *gmask,
	uint32_t *bmask);

} // namespace boot
