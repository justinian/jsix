#include <efi.h>
#include <efilib.h>

#include "utility.h"

EFI_STATUS
set_graphics_mode (EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS status;

	Print(L"Setting console display mode... ");

	EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx_out_proto;
	EFI_GUID gfx_out_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

	status = SystemTable->BootServices->LocateProtocol(
		&gfx_out_guid, NULL, (void**)&gfx_out_proto);
	CHECK_EFI_STATUS_OR_RETURN(status, "LocateProtocol gfx");

	const uint32_t modes = gfx_out_proto->Mode->MaxMode;
	uint32_t res =
		gfx_out_proto->Mode->Info->HorizontalResolution *
		gfx_out_proto->Mode->Info->VerticalResolution;
	uint32_t best = (uint32_t)-1;

	for (uint32_t i = 0; i < modes; ++i) {
		UINTN size = 0;
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = NULL;
		status = gfx_out_proto->QueryMode(gfx_out_proto, i, &size, &info);
		CHECK_EFI_STATUS_OR_RETURN(status, "QueryMode");

		const uint32_t new_res =
			info->HorizontalResolution *
			info->VerticalResolution;

		if (new_res > res) {
			best = i;
			res = new_res;
		}
	}

	if (best != (uint32_t)-1) {
		status = gfx_out_proto->SetMode(gfx_out_proto, best);
		CHECK_EFI_STATUS_OR_RETURN(status, "SetMode");
		Print(L"*");
	}
	Print(L"%ux%u\n",
		gfx_out_proto->Mode->Info->HorizontalResolution,
		gfx_out_proto->Mode->Info->VerticalResolution);

	return status;
}

