#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <uefi/boot_services.h>
#include <uefi/protos/simple_text_output.h>

namespace boot {

class console
{
public:
	console(uefi::boot_services *bs, uefi::protos::simple_text_output *out);

	size_t print_hex(uint32_t n) const;
	size_t print_dec(uint32_t n) const;
	size_t print_long_hex(uint64_t n) const;
	size_t print_long_dec(uint64_t n) const;
	size_t printf(const wchar_t *fmt, ...) const;

	static console & get() { return *s_console; }
	static size_t print(const wchar_t *fmt, ...);

private:
	friend class status_line;

	void pick_mode(uefi::boot_services *bs);
	size_t vprintf(const wchar_t *fmt, va_list args) const;

	size_t m_rows, m_cols;
	uefi::protos::simple_text_output *m_out;

	static console *s_console;
};

class status_line
{
public:
	status_line(const wchar_t *message, const wchar_t *context = nullptr);
	~status_line();

	inline static void warn(const wchar_t *message, const wchar_t *error = nullptr) {
		if (s_current) s_current->do_warn(message, error);
	}

	inline static void fail(const wchar_t *message, const wchar_t *error = nullptr) {
		if (s_current) s_current->do_fail(message, error);
	}

private:
	void print_status_tag();
	void do_warn(const wchar_t *message, const wchar_t *error);
	void do_fail(const wchar_t *message, const wchar_t *error);
	void finish();

	size_t m_line;
	int m_level;
	int m_depth;

	status_line *m_next;
	static status_line *s_current;
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
