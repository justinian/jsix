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

const wchar_t * message(uefi::status status);

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

