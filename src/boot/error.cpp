#include "error.h"
#include "console.h"
#include "kernel_args.h"
#include "status.h"

namespace boot {
namespace error {

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
message(uefi::status status)
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

[[ noreturn ]] static void
cpu_assert(uefi::status s, const wchar_t *message)
{
	asm volatile (
		"movq $0xeeeeeeebadbadbad, %%r8;"
		"movq %0, %%r9;"
		"movq %1, %%r10;"
		"movq $0, %%rdx;"
		"divq %%rdx;"
		:
		: "r"((uint64_t)s), "r"(message)
		: "rax", "rdx", "r8", "r9", "r10");
	while (1) asm("hlt");
}

[[ noreturn ]] void
raise(uefi::status status, const wchar_t *message)
{
	if(status_line::fail(message, status))
		while (1) asm("hlt");
	else
		cpu_assert(status, message);
}


} // namespace error
} // namespace boot

void debug_break()
{
	volatile int go = 0;
	while (!go);
}
