#include <stddef.h>

#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "log.h"
#include "msr.h"
#include "scheduler.h"
#include "syscall.h"

extern "C" {
	void syscall_invalid(uint64_t call);
}

uintptr_t syscall_registry[256] __attribute__((section(".syscall_registry")));
const char * syscall_names[256] __attribute__((section(".syscall_registry")));
static constexpr size_t num_syscalls = sizeof(syscall_registry) / sizeof(syscall_registry[0]);

void
syscall_invalid(uint64_t call)
{
	console *cons = console::get();
	cons->set_color(9);
	cons->printf("\nReceived unknown syscall: %02x\n", call);

	cons->printf("  Known syscalls:\n");
		cons->printf("          invalid %016lx\n", syscall_invalid);

	for (unsigned i = 0; i < num_syscalls; ++i) {
		const char *name = syscall_names[i];
		uintptr_t handler = syscall_registry[i];
		if (name)
			cons->printf("    %02x %10s %016lx\n", i, name, handler);
	}

	cons->set_color();
	_halt();
}

void
syscall_initialize()
{
	kutil::memset(&syscall_registry, 0, sizeof(syscall_registry));
	kutil::memset(&syscall_names, 0, sizeof(syscall_names));

#define SYSCALL(id, name, result, ...) \
	syscall_registry[id] = reinterpret_cast<uintptr_t>(syscalls::name); \
	syscall_names[id] = #name; \
	log::debug(logs::syscall, "Enabling syscall 0x%02x as " #name , id);
#include "j6/tables/syscalls.inc"
#undef SYSCALL
}

