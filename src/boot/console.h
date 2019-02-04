#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <efi/efi.h>

class console
{
public:
	console(EFI_SYSTEM_TABLE *system_table);

	EFI_STATUS initialize(const wchar_t *version);

	void status_begin(const wchar_t *message) const;
	void status_fail(const wchar_t *error) const;
	void status_ok() const;

	size_t print_hex(uint32_t n) const;
	size_t print_dec(uint32_t n) const;
	size_t print_long_hex(uint64_t n) const;
	size_t print_long_dec(uint64_t n) const;
	size_t printf(const wchar_t *fmt, ...) const;

	static const console & get() { return *s_console; }
	static size_t print(const wchar_t *fmt, ...);

private:
	EFI_STATUS pick_mode();
	size_t vprintf(const wchar_t *fmt, va_list args) const;

	size_t m_rows, m_cols;
	EFI_BOOT_SERVICES *m_boot;
	EFI_SIMPLE_TEXT_OUT_PROTOCOL *m_out;

	static console *s_console;
};

EFI_STATUS
con_get_framebuffer(
	EFI_BOOT_SERVICES *bootsvc,
	void **buffer,
	size_t *buffer_size,
	uint32_t *hres,
	uint32_t *vres,
	uint32_t *rmask,
	uint32_t *gmask,
	uint32_t *bmask);
