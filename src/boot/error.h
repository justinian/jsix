/// \file error.h
/// Error handling definitions
#pragma once

#include <stddef.h>
#include <uefi/types.h>

namespace boot {

class console;

namespace error {

/// Halt or exit the program with the given error status/message
[[ noreturn ]] void raise(uefi::status status, const wchar_t *message);

/// Interface for error-handling functors
class handler
{
public:
	/// Constructor must be called by implementing classes.
	handler();
	virtual ~handler();

	/// Interface for implementations of error handling.
	virtual void handle(uefi::status, const wchar_t*) = 0;

private:
	friend void raise(uefi::status, const wchar_t *);

	handler *m_next;
	static handler *s_current;
};

/// Error handler using UEFI boot services. Integrates with `status_line`
/// to print formatted error messages to the screen.
class uefi_handler :
	public handler
{
public:
	uefi_handler(console &con);
	[[ noreturn ]] void handle(uefi::status, const wchar_t*) override;

private:
	console &m_con;
};

/// Error handler that doesn't rely on UEFI. Sets status into CPU
/// registers and then causes a CPU #DE exception.
class cpu_assert_handler :
	public handler
{
public:
	cpu_assert_handler();
	[[ noreturn ]] void handle(uefi::status, const wchar_t*) override;
};

} // namespace error
} // namespace boot

/// Debugging psuedo-breakpoint.
void debug_break();

/// Helper macro to raise an error if an operation fails.
/// \arg s  An expression evaluating to a UEFI status
/// \arg m  The error message to use on failure
#define try_or_raise(s, m) \
	do { \
		uefi::status _s = (s); \
		if (uefi::is_error(_s)) ::boot::error::raise(_s, (m)); \
	} while(0)

