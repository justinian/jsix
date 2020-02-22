#include "error.h"
#include "console.h"

namespace boot {
namespace error {

handler *handler::s_current = nullptr;

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
	m_con.status_fail(message);
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

extern "C" int _purecall() { ::boot::error::raise(uefi::status::unsupported, L"Pure virtual call"); }
void operator delete (void *) {}

