#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "scheduler.h"
#include "syscall.h"

extern "C" {
	void _halt();
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
			p->flags -= process_flags::ready;
			//log::debug(logs::task, "Pausing process %d, flags: %08x", p->pid, p->flags);
			cons->printf("\nReceived PAUSE syscall\n");
			return_rsp = s.tick(return_rsp);
			//log::debug(logs::task, "Switching to stack %016lx", return_rsp);
			cons->printf("\nDONE WITH PAUSE syscall\n");
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

