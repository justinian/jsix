#pragma once
#include <efi.h>
#include <stddef.h>

EFI_STATUS con_initialize(EFI_SYSTEM_TABLE *system_table, const CHAR16 *version);
void con_status_begin(const CHAR16 *message);
void con_status_ok();
void con_status_fail(const CHAR16 *error);
size_t con_printf(const CHAR16 *fmt, ...);
