#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "msr.h"
#include "process.h"
#include "scheduler.h"
#include "syscall.h"

extern "C" {
	void _halt();
	void syscall_handler_prelude();
}

void
syscall_enable()
{
	// IA32_EFER - set bit 0, syscall enable
	uint64_t efer = rdmsr(msr::ia32_efer);
	wrmsr(msr::ia32_efer, efer|1);

	// IA32_STAR - high 32 bits contain k+u CS
	// Kernel CS: GDT[1] ring 0 bits[47:32]
	//   User CS: GDT[3] ring 3 bits[63:48]
	uint64_t star =
		(((1ull << 3) | 0) << 32) |
		(((3ull << 3) | 3) << 48);
	wrmsr(msr::ia32_star, star);

	// IA32_LSTAR - RIP for syscall
	wrmsr(msr::ia32_lstar,
		reinterpret_cast<addr_t>(&syscall_handler_prelude));

	// IA32_FMASK - FLAGS mask inside syscall
	wrmsr(msr::ia32_fmask, 0x200);
}

addr_t
syscall_dispatch(addr_t return_rsp, const cpu_state &regs)
{
	console *cons = console::get();
	syscall call = static_cast<syscall>(regs.rax);

	switch (call) {
	case syscall::noop:
		break;

	case syscall::debug:
		cons->set_color(11);
		cons->printf("\nReceived DEBUG syscall\n");
		cons->set_color();
		print_regs(regs);
		break;

	case syscall::message:
		cons->set_color(11);
		cons->printf("\nReceived MESSAGE syscall\n");
		cons->set_color();
		break;

	case syscall::pause:
		{
			cons->set_color(11);

			auto &s = scheduler::get();
			auto *p = s.current();
			p->wait_on_signal(-1ull);
			cons->printf("\nReceived PAUSE syscall\n");
			return_rsp = s.tick(return_rsp);
			cons->set_color();
		}
		break;

	case syscall::sleep:
		{
			cons->set_color(11);

			auto &s = scheduler::get();
			auto *p = s.current();
			p->wait_on_time(regs.rbx);
			cons->printf("\nReceived SLEEP syscall\n");
			return_rsp = s.tick(return_rsp);
			cons->set_color();
		}
		break;

	default:
		cons->set_color(9);
		cons->printf("\nReceived unknown syscall: %02x\n", call);
		cons->set_color();
		_halt();
		break;
	}

	return return_rsp;
}

