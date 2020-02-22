#include "utility.h"

struct error_code_desc {
	uefi::status code;
	const wchar_t *name;
};

struct error_code_desc error_table[] = {
#define STATUS_ERROR(name, num) { uefi::status::name, L#name },
#define STATUS_WARNING(name, num) { uefi::status::name, L#name },
#include "uefi/errors.inc"
#undef STATUS_ERROR
#undef STATUS_WARNING
	{ uefi::status::success, nullptr }
};

const wchar_t *
util_error_message(uefi::status status)
{
	int32_t i = -1;
	while (error_table[++i].name != nullptr) {
		if (error_table[i].code == status) return error_table[i].name;
	}

	if (uefi::is_error(status))
		return L"Unknown Error";
	else
		return L"Unknown Warning";
}

size_t
wstrlen(const wchar_t *s)
{
	size_t count = 0;
	while (s && *s++) count++;
	return count;
}

