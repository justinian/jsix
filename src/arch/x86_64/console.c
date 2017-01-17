#include <efi.h>
#include <efilib.h>
#include <stdint.h>

#include "console.h"
#include "utility.h"

UINTN ROWS = 0;
UINTN COLS = 0;

EFI_STATUS
con_initialize (const CHAR16 *version)
{
	EFI_STATUS status;

	Print(L"Setting console display mode...\n");

	EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx_out_proto;
	EFI_GUID gfx_out_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

	status = ST->BootServices->LocateProtocol(
		&gfx_out_guid, NULL, (void**)&gfx_out_proto);
	CHECK_EFI_STATUS_OR_RETURN(status, "LocateProtocol gfx");

	const uint32_t modes = gfx_out_proto->Mode->MaxMode;
	uint32_t res =
		gfx_out_proto->Mode->Info->HorizontalResolution *
		gfx_out_proto->Mode->Info->VerticalResolution;
	uint32_t best = gfx_out_proto->Mode->Mode;

	for (uint32_t i = 0; i < modes; ++i) {
		UINTN size = 0;
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = NULL;
		status = gfx_out_proto->QueryMode(gfx_out_proto, i, &size, &info);
		CHECK_EFI_STATUS_OR_RETURN(status, "QueryMode");

#ifdef MAX_HRES
		if (info->HorizontalResolution > MAX_HRES)
			continue;
#endif

		const uint32_t new_res =
			info->HorizontalResolution *
			info->VerticalResolution;

		if (new_res > res) {
			best = i;
			res = new_res;
		}
	}

	status = gfx_out_proto->SetMode(gfx_out_proto, best);
	CHECK_EFI_STATUS_OR_RETURN(status, "SetMode %d/%d", best, modes);

	status = ST->ConOut->QueryMode(
		ST->ConOut,
		ST->ConOut->Mode->Mode,
		&COLS, &ROWS);
	CHECK_EFI_STATUS_OR_RETURN(status, "QueryMode");

	status = ST->ConOut->ClearScreen(ST->ConOut);
	CHECK_EFI_STATUS_OR_RETURN(status, "ClearScreen");

	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTCYAN);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)L"Popcorn OS ");

	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTMAGENTA);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)version);

	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)L" booting...\r\n");

	con_status_begin(L"Setting console display mode: ");
	Print(L"%ux%u (%ux%u chars)\n",
		gfx_out_proto->Mode->Info->HorizontalResolution,
		gfx_out_proto->Mode->Info->VerticalResolution,
		ROWS, COLS);
	con_status_ok();

	return status;
}

void
con_status_begin (const CHAR16 *message)
{
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)message);
}

void
con_status_ok ()
{
	UINTN row = ST->ConOut->Mode->CursorRow;
	ST->ConOut->SetCursorPosition(ST->ConOut, COLS - 8, row - 1);
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)L"[");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_GREEN);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)L"  ok  ");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)L"]\r");
}

void
con_status_fail (const CHAR16 *error)
{
	UINTN row = ST->ConOut->Mode->CursorRow;
	ST->ConOut->SetCursorPosition(ST->ConOut, COLS - 8, row - 1);
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)L"[");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTRED);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)L"failed");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)L"]\r\n");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_RED);
	ST->ConOut->OutputString(ST->ConOut, (CHAR16*)error);
}