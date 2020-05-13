#include "error.h"
#include "console.h"

namespace boot {
namespace error {

handler *handler::s_current = nullptr;

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

static const wchar_t *
error_message(uefi::status status)
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

[[ noreturn ]] void
raise(uefi::status status, const wchar_t *message)
{
	if (handler::s_current) {
		handler::s_current->handle(status, message);
	}
	while (1) asm("hlt");
}

handler::handler() :
	m_next(s_current)
{
	s_current = this;
}

handler::~handler()
{
	if (s_current != this)
		raise(uefi::status::warn_stale_data,
				L"Non-current error handler destructing");
	s_current = m_next;
}

uefi_handler::uefi_handler(console &con) :
	handler(),
	m_con(con)
{
}

[[ noreturn ]] void
uefi_handler::handle(uefi::status s, const wchar_t *message)
{
	status_line::fail(message, error_message(s));
	while (1) asm("hlt");
}

cpu_assert_handler::cpu_assert_handler() : handler() {}

[[ noreturn ]] void
cpu_assert_handler::handle(uefi::status s, const wchar_t *message)
{
	asm volatile (
		"movq $0xeeeeeeebadbadbad, %%r8;"
		"movq %0, %%r9;"
		"movq $0, %%rdx;"
		"divq %%rdx;"
		:
		: "r"((uint64_t)s)
		: "rax", "rdx", "r8", "r9");
	while (1) asm("hlt");
}

} // namespace error
} // namespace boot

void debug_break()
{
	volatile int go = 0;
	while (!go);
}
