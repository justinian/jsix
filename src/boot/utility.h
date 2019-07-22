#include <stddef.h>
#include <efi/efi.h>

#include "console.h"

#define UNUSED __attribute__((unused))

size_t wstrlen(const wchar_t *s);
const wchar_t *util_error_message(EFI_STATUS status);

#define CHECK_EFI_STATUS_OR_RETURN(s, msg, ...) \
	if (EFI_ERROR((s))) { \
		console::print(L"ERROR: " msg L": %s\r\n", ##__VA_ARGS__, util_error_message(s)); \
		return (s); \
	}

#define CHECK_EFI_STATUS_OR_FAIL(s) \
	if (EFI_ERROR((s))) { \
		console::get().status_fail(util_error_message(s)); \
		while (1) __asm__("hlt"); \
	}

#define CHECK_EFI_STATUS_OR_ASSERT(s, d) \
	if (EFI_ERROR((s))) { \
		__asm__ __volatile__( \
			"movq %0, %%r8;" \
			"movq %1, %%r9;" \
			"movq %2, %%r10;" \
			"movq $0, %%rdx;" \
			"divq %%rdx;" \
			: \
			: "r"((uint64_t)s), "r"((uint64_t)d), "r"((uint64_t)__LINE__) \
			: "rax", "rdx", "r8", "r9", "r10"); \
	}

#ifdef BOOTLOADER_DEBUG
#define con_debug(msg, ...) console::print(L"DEBUG: " msg L"\r\n", __VA_ARGS__)
#else
#define con_debug(msg, ...)
#endif
