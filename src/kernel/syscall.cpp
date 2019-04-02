#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "log.h"
#include "msr.h"
#include "process.h"
#include "scheduler.h"
#include "syscall.h"

extern "C" {
	void _halt();
	void syscall_handler_prelude();
}

namespace syscalls {

void
noop()
{
	auto &s = scheduler::get();
	auto *p = s.current();
	log::debug(logs::syscall, "Process %d called noop syscall.", p->pid);
}

void
exit(int64_t status)
{
	auto &s = scheduler::get();
	auto *p = s.current();
	log::debug(logs::syscall, "Process %d exiting with code %d", p->pid, status);

	p->exit(status);
	s.schedule();
}

pid_t
getpid()
{
	auto &s = scheduler::get();
	auto *p = s.current();
	return p->pid;
}

pid_t fork() { return 0; }

void
message(const char *message)
{
	auto &s = scheduler::get();
	auto *p = s.current();
	log::info(logs::syscall, "Message[%d]: %s", p->pid, message);
}

void
pause()
{
	auto &s = scheduler::get();
	auto *p = s.current();
	p->wait_on_signal(-1ull);
	s.schedule();
}

void
sleep(uint64_t til)
{
	auto &s = scheduler::get();
	auto *p = s.current();
	log::debug(logs::syscall, "Process %d sleeping until %d", p->pid, til);

	p->wait_on_time(til);
	s.schedule();
}

void send() {}
void receive() {}

} // namespace syscalls

struct syscall_handler_info
{
	unsigned nargs;
	const char *name;
};

uintptr_t syscall_registry[static_cast<unsigned>(syscall::COUNT)];
syscall_handler_info syscall_info_registry[static_cast<unsigned>(syscall::COUNT)];

void
syscall_invalid(uint64_t call)
{
	console *cons = console::get();
	cons->set_color(9);
	cons->printf("\nReceived unknown syscall: %d\n", call);
	cons->set_color();
	_halt();
}

/*
void
syscall_dispatch(cpu_state *regs)
{
	console *cons = console::get();
	syscall call = static_cast<syscall>(regs->rax);

	auto &s = scheduler::get();
	auto *p = s.current();

	switch (call) {
	case syscall::noop:
		break;

	case syscall::debug:
		cons->set_color(11);
		cons->printf("\nProcess %d: Received DEBUG syscall\n", p->pid);
		cons->set_color();
		print_regs(*regs);
		cons->printf("\n         Syscall enters: %8d\n", __counter_syscall_enter);
		cons->printf("         Syscall sysret: %8d\n", __counter_syscall_sysret);
		break;

	case syscall::send:
		{
			pid_t target = regs->rdi;
			uintptr_t data = regs->rsi;

			cons->set_color(11);
			cons->printf("\nProcess %d: Received SEND syscall, target %d, data %016lx\n", p->pid, target, data);
			cons->set_color();

			if (p->wait_on_send(target))
				s.schedule();
		}
		break;

	case syscall::receive:
		{
			pid_t source = regs->rdi;
			uintptr_t data = regs->rsi;

			cons->set_color(11);
			cons->printf("\nProcess %d: Received RECEIVE syscall, source %d, dat %016lx\n", p->pid, source, data);
			cons->set_color();

			if (p->wait_on_receive(source))
				s.schedule();
		}
		break;

	case syscall::fork:
		{
			cons->set_color(11);
			cons->printf("\nProcess %d: Received FORK syscall\n", p->pid);
			cons->set_color();

			pid_t pid = p->fork(regs);
			cons->printf("\n   fork returning %d\n", pid);
			regs->rax = pid;
		}
		break;

	default:
		cons->set_color(9);
		cons->printf("\nReceived unknown syscall: %02x\n", call);
		cons->set_color();
		_halt();
		break;
	}
}
*/

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

#define SYSCALL(name, nargs) \
	syscall_registry[static_cast<unsigned>(syscall::name)] = \
		reinterpret_cast<uintptr_t>(syscalls::name); \
	syscall_info_registry[static_cast<unsigned>(syscall::name)] = { \
		nargs, #name };
#include "syscalls.inc"
#undef SYSCALL
}

