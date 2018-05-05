#pragma once
#include <efi/efi.h>

int is_guid(EFI_GUID *a, EFI_GUID *b);

#define GUID(dw, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, name) extern EFI_GUID name
#include "guids.inc"
#undef GUID
