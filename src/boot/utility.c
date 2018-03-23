#include <efi.h>

struct ErrorCode {
	EFI_STATUS code;
	const CHAR16 *desc;
};

extern struct ErrorCode ErrorCodeTable[];

const CHAR16 *util_error_message(EFI_STATUS status) {
	int32_t i = -1;
	while (ErrorCodeTable[++i].desc != NULL) {
		if (ErrorCodeTable[i].code == status)
			return ErrorCodeTable[i].desc;
	}

	return L"Unknown";
}
