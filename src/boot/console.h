#pragma once
#include <efi.h>

EFI_STATUS con_initialize (const CHAR16 *version);
void con_status_begin (const CHAR16 *message);
void con_status_ok ();
void con_status_fail (const CHAR16 *error);
