#include <efi.h>
#include <efilib.h>

#include "graphics.h"
#include "utility.h"

EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS status;

	InitializeLib(ImageHandle, SystemTable);

	Print(L"Popcorn OS " GIT_VERSION L" booting...\n");

	status = set_graphics_mode(SystemTable);
	CHECK_EFI_STATUS_OR_RETURN(status, "set_graphics_mode");

	Print(L" SystemTable: %x\n", SystemTable);
	if (SystemTable)
		Print(L"      ConOut: %x\n", SystemTable->ConOut);
	if (SystemTable->ConOut)
		Print(L"OutputString: %x\n", SystemTable->ConOut->OutputString);

	while (1) __asm__("hlt");
	return status;
}

