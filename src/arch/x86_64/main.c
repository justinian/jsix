#include <efi.h>
#include <efilib.h>

#include "console.h"
#include "utility.h"

#ifndef GIT_VERSION
	#define GIT_VERSION L"no version"
#endif

EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS status;

	InitializeLib(ImageHandle, SystemTable);

	// When checking the console initialization error,
	// use CHECK_EFI_STATUS_OR_RETURN because we can't
	// be sure if the console was fully set up
	status = con_initialize(GIT_VERSION);
	CHECK_EFI_STATUS_OR_RETURN(status, "con_initialize");

	// From here on out, use CHECK_EFI_STATUS_OR_FAIL instead
	// because the console is now set up

	Print(L" SystemTable: %x\n", SystemTable);
	if (SystemTable)
		Print(L"      ConOut: %x\n", SystemTable->ConOut);
	if (SystemTable->ConOut)
		Print(L"OutputString: %x\n", SystemTable->ConOut->OutputString);

	while (1) __asm__("hlt");
	return status;
}

