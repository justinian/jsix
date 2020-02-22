#include <stdint.h>
#include <uefi/types.h>

#include "console.h"

#define UNUSED __attribute__((unused))

size_t wstrlen(const wchar_t *s);

#ifdef BOOTLOADER_DEBUG
#define con_debug(msg, ...) console::print(L"DEBUG: " msg L"\r\n", __VA_ARGS__)
#else
#define con_debug(msg, ...)
#endif
