#include <efi.h>
#include <efilib.h>
#include "console.h"

#define UNUSED __attribute__((unused))

const CHAR16 *util_error_message(EFI_STATUS status);

#define CHECK_EFI_STATUS_OR_RETURN(s, msg, ...)   \
	if (EFI_ERROR((s))) { \
		Print(L"EFI_ERROR: " msg L": %s\n", ## __VA_ARGS__, util_error_message(s)); \
		return (s); \
	}

#define CHECK_EFI_STATUS_OR_FAIL(s)   \
	if (EFI_ERROR((s))) { \
		con_status_fail(util_error_message(s)); \
		while (1) __asm__("hlt"); \
	}

