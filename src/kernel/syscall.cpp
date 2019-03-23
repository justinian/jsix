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
		reinterpret_cast<uintptr_t>(&syscall_handler_prelude));

	// IA32_FMASK - FLAGS mask inside syscall
	wrmsr(msr::ia32_fmask, 0x200);
}

uintptr_t
syscall_dispatch(uintptr_t return_rsp, cpu_state &regs)
{
	console *cons = console::get();
	syscall call = static_cast<syscall>(regs.rax);

	auto &s = scheduler::get();
	auto *p = s.current();

	switch (call) {
	case syscall::noop:
		break;

	case syscall::debug:
		cons->set_color(11);
		cons->printf("\nProcess %d: Received DEBUG syscall\n", p->pid);
		cons->set_color();
		print_regs(regs);
		cons->printf("\n         Syscall enters: %8d\n", __counter_syscall_enter);
		cons->printf("         Syscall sysret: %8d\n", __counter_syscall_sysret);
		break;

	case syscall::message:
		cons->set_color(11);
		cons->printf("\nProcess %d: Received MESSAGE syscall\n", p->pid);
		cons->set_color();
		break;

	case syscall::pause:
		{
			cons->set_color(11);

			auto &s = scheduler::get();
			auto *p = s.current();
			p->wait_on_signal(-1ull);
			cons->printf("\nProcess %d: Received PAUSE syscall\n", p->pid);
			cons->set_color();
			return_rsp = s.schedule(return_rsp);
		}
		break;

	case syscall::sleep:
		{
			cons->set_color(11);
			cons->printf("\nProcess %d: Received SLEEP syscall\n", p->pid);
			cons->printf("Sleeping until %lu\n", regs.rbx);
			cons->set_color();

			p->wait_on_time(regs.rbx);
			return_rsp = s.schedule(return_rsp);
		}
		break;

	case syscall::getpid:
		cons->set_color(11);
		cons->printf("\nProcess %d: Received GETPID syscall\n", p->pid);
		cons->set_color();
		regs.rax = p->pid;
		break;

	case syscall::send:
		{
			pid_t target = regs.rdi;
			uintptr_t data = regs.rsi;

			cons->set_color(11);
			cons->printf("\nProcess %d: Received SEND syscall, target %d, data %016lx\n", p->pid, target, data);
			cons->set_color();

			if (p->wait_on_send(target))
				return_rsp = s.schedule(return_rsp);
		}
		break;

	case syscall::receive:
		{
			pid_t source = regs.rdi;
			uintptr_t data = regs.rsi;

			cons->set_color(11);
			cons->printf("\nProcess %d: Received RECEIVE syscall, source %d, dat %016lx\n", p->pid, source, data);
			cons->set_color();

			if (p->wait_on_receive(source))
				return_rsp = s.schedule(return_rsp);
		}
		break;

	case syscall::fork:
		{
			cons->set_color(11);
			cons->printf("\nProcess %d: Received FORK syscall\n", p->pid);
			cons->set_color();

			pid_t pid = p->fork(return_rsp);
			regs.rax = pid;
		}
		break;

	case syscall::exit:
		cons->set_color(11);
		cons->printf("\nProcess %d: Received EXIT syscall\n", p->pid);
		cons->set_color();
		p->exit(regs.rdi);
		return_rsp = s.schedule(return_rsp);
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

