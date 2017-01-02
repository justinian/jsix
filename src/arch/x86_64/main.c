//#define EFIAPI __attribute__((ms_abi))

#include <efi.h>
#include <efilib.h>

#define check_status(s, msg)   if(EFI_ERROR((s))){Print(L"EFI_ERROR: " msg L" %d\n", (s)); return (s);}

EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS status;

	InitializeLib(ImageHandle, SystemTable);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
	status = ST->ConOut->OutputString(ST->ConOut, L"Hello from UEFI\n\r");
#pragma GCC diagnostic pop
	check_status(status, L"OutputString");

	Print(L" SystemTable: %x\n", SystemTable);
	if (SystemTable)
		Print(L"      ConOut: %x\n", SystemTable->ConOut);
	if (SystemTable->ConOut)
		Print(L"OutputString: %x\n", SystemTable->ConOut->OutputString);

	while (1) __asm__("hlt");
	return status;
}

