#pragma once
#include <stddef.h>
#include <efi/efi.h>

EFI_STATUS con_initialize(EFI_SYSTEM_TABLE *system_table, const CHAR16 *version);
void con_status_begin(const CHAR16 *message);
void con_status_ok();
void con_status_fail(const CHAR16 *error);
size_t con_printf(const CHAR16 *fmt, ...);

EFI_STATUS
con_get_framebuffer(
	EFI_BOOT_SERVICES *bootsvc,
	void **buffer,
	size_t *buffer_size,
	uint32_t *hres,
	uint32_t *vres,
	uint32_t *rmask,
	uint32_t *gmask,
	uint32_t *bmask);
