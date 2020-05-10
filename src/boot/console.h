/// \file console.h
/// Text output and status message handling
#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <uefi/boot_services.h>
#include <uefi/protos/simple_text_output.h>

namespace boot {

/// Object providing basic output functionality to the UEFI console
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

/// Scoped status line reporter. Prints a message and an "OK" if no errors
/// or warnings were reported before destruction, otherwise reports the
/// error or warning.
class status_line
{
public:
	/// Constructor.
	/// \arg message  Description of the operation in progress
	/// \arg context  If non-null, printed after `message` and a colon
	status_line(const wchar_t *message, const wchar_t *context = nullptr);
	~status_line();

	/// Set the state to warning, and print a message. If the state is already at
	/// warning or error, the state is unchanged but the message is still printed.
	/// \arg message  The warning message to print
	/// \arg error    If non-null, printed after `message`
	inline static void warn(const wchar_t *message, const wchar_t *error = nullptr) {
		if (s_current) s_current->do_warn(message, error);
	}

	/// Set the state to error, and print a message. If the state is already at
	/// error, the state is unchanged but the message is still printed.
	/// \arg message  The error message to print
	/// \arg error    If non-null, printed after `message`
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

} // namespace boot
