#include <efi.h>
#include <efilib.h>

#define CHECK_EFI_STATUS_OR_RETURN(s, msg)   \
	if (EFI_ERROR((s))) { \
		Print(L"EFI_ERROR: " msg L" %d\n", (s)); \
		return (s); \
	}

