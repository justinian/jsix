#include <stdint.h>

#include "console.h"
#include "cpu.h"
#include "device_manager.h"
#include "gdt.h"
#include "interrupts.h"
#include "io.h"
#include "log.h"
#include "scheduler.h"
#include "syscall.h"

extern "C" {
	void _halt();

	addr_t isr_handler(addr_t, cpu_state);
	addr_t irq_handler(addr_t, cpu_state);
	addr_t syscall_handler(addr_t, cpu_state);

#define ISR(i, name)     extern void name ();
#define EISR(i, name)    extern void name ();
#define IRQ(i, q, name)  extern void name ();
#include "interrupt_isrs.inc"
#undef IRQ
#undef EISR
#undef ISR
}

isr
operator+(const isr &lhs, int rhs)
{
	using under_t = std::underlying_type<isr>::type;
	return static_cast<isr>(static_cast<under_t>(lhs) + rhs);
}

uint8_t
get_irq(unsigned vector)
{
	switch (vector) {
#define ISR(i, name)
#define EISR(i, name)
#define IRQ(i, q, name) case i : return q;
#include "interrupt_isrs.inc"
#undef IRQ
#undef EISR
#undef ISR

		default: return 0xff;
	}
}

static void
disable_legacy_pic()
{

	static const uint16_t PIC1 = 0x20;
	static const uint16_t PIC2 = 0xa0;

	// Mask all interrupts
	outb(0xa1, 0xff);
	outb(0x21, 0xff);

	// Start initialization sequence
	outb(PIC1, 0x11); io_wait();
	outb(PIC2, 0x11); io_wait();

	// Remap into ignore ISRs
	outb(PIC1+1, static_cast<uint8_t>(isr::isrIgnore0)); io_wait();
	outb(PIC2+1, static_cast<uint8_t>(isr::isrIgnore0)); io_wait();

	// Tell PICs about each other
	outb(PIC1+1, 0x04); io_wait();
	outb(PIC2+1, 0x02); io_wait();
}

static void
enable_serial_interrupts()
{
	uint8_t ier = inb(COM1+1);
	outb(COM1+1, ier | 0x1);
}

void
interrupts_init()
{
#define ISR(i, name)     idt_set_entry(i, reinterpret_cast<uint64_t>(& name), 0x08, 0x8e);
#define EISR(i, name)    idt_set_entry(i, reinterpret_cast<uint64_t>(& name), 0x08, 0x8e);
#define IRQ(i, q, name)  idt_set_entry(i, reinterpret_cast<uint64_t>(& name), 0x08, 0x8e);
#include "interrupt_isrs.inc"
#undef IRQ
#undef EISR
#undef ISR

	disable_legacy_pic();
	enable_serial_interrupts();

	log::info(logs::boot, "Interrupts enabled.");
}

#define print_reg(name, value) cons->printf("         %s: %016lx\n", name, (value));

extern "C" uint64_t get_frame(int frame);

void
print_stacktrace(int skip = 0)
{
	console *cons = console::get();
	int frame = 0;
	uint64_t bp = get_frame(skip);
	while (bp) {
		cons->printf("    frame %2d: %lx\n", frame, bp);
		bp = get_frame(++frame + skip);
	}
}

void
print_regs(const cpu_state &regs)
{
	console *cons = console::get();

	print_reg("rax", regs.rax);
	print_reg("rbx", regs.rbx);
	print_reg("rcx", regs.rcx);
	print_reg("rdx", regs.rdx);
	print_reg("rdi", regs.rdi);
	print_reg("rsi", regs.rsi);

	cons->puts("\n");
	print_reg(" r8", regs.r8);
	print_reg(" r9", regs.r9);
	print_reg("r10", regs.r10);
	print_reg("r11", regs.r11);
	print_reg("r12", regs.r12);
	print_reg("r13", regs.r13);
	print_reg("r14", regs.r14);
	print_reg("r15", regs.r15);

	cons->puts("\n");
	print_reg("rbp", regs.rbp);
	print_reg("rsp", regs.user_rsp);
	print_reg("sp0", tss_get_stack(0));

	cons->puts("\n");
	print_reg(" ds", regs.ds);
	print_reg(" cs", regs.cs);
	print_reg(" ss", regs.ss);

	cons->puts("\n");
	print_reg("rip", regs.rip);
}

void
print_stack(const cpu_state &regs)
{
	console *cons = console::get();

	cons->puts("\nStack:\n");
	uint64_t sp = regs.user_rsp;
	while (sp <= regs.rbp) {
		cons->printf("%016x: %016x\n", sp, *reinterpret_cast<uint64_t *>(sp));
		sp += sizeof(uint64_t);
	}
}

addr_t
isr_handler(addr_t return_rsp, cpu_state regs)
{
	console *cons = console::get();

	switch (static_cast<isr>(regs.interrupt & 0xff)) {
	case isr::isrTimer: {
			scheduler &s = scheduler::get();
			return_rsp = s.tick(return_rsp);
		}
		break;

	case isr::isrLINT0:
		cons->puts("\nLINT0\n");
		break;

	case isr::isrLINT1:
		cons->puts("\nLINT1\n");
		break;

	case isr::isrIgnore0:
	case isr::isrIgnore1:
	case isr::isrIgnore2:
	case isr::isrIgnore3:
	case isr::isrIgnore4:
	case isr::isrIgnore5:
	case isr::isrIgnore6:
	case isr::isrIgnore7:
		break;

	case isr::isrGPFault: {
			cons->set_color(9);
			cons->puts("\nGeneral Protection Fault:\n");
			cons->set_color();

			cons->puts("       flags:");
			if (regs.errorcode & 0x01) cons->puts(" external");

			int index = (regs.errorcode & 0xf8) >> 3;
			if (index) {
				switch (regs.errorcode & 0x06) {
				case 0:
					cons->printf(" GDT[%d]\n", index);
					gdt_dump();
					break;

				case 1:
				case 3:
					cons->printf(" IDT[%d]\n", index);
					idt_dump();
					break;

				default:
					cons->printf(" LDT[%d]??\n", index);
					break;
				}
			} else {
				cons->putc('\n');
			}
			print_regs(regs);
			print_stacktrace(2);
			print_stack(regs);

		}
		_halt();
		break;

	case isr::isrPageFault: {
			cons->set_color(11);
			cons->puts("\nPage Fault:\n");
			cons->set_color();

			cons->puts("       flags:");
			if (regs.errorcode & 0x01) cons->puts(" present");
			if (regs.errorcode & 0x02) cons->puts(" write");
			if (regs.errorcode & 0x04) cons->puts(" user");
			if (regs.errorcode & 0x08) cons->puts(" reserved");
			if (regs.errorcode & 0x10) cons->puts(" ip");
			cons->puts("\n");

			uint64_t cr2 = 0;
			__asm__ __volatile__ ("mov %%cr2, %0" : "=r"(cr2));
			print_reg("cr2", cr2);

			print_reg("rsp", regs.user_rsp);
			print_reg("rip", regs.rip);

			cons->puts("\n");
			print_stacktrace(2);
		}
		_halt();
		break;

	case isr::isrAssert: {
			cons->set_color();
			print_regs(regs);
			print_stacktrace(2);
		}
		_halt();
		break;

	default:
		cons->set_color(9);
		cons->puts("\nReceived ISR interrupt:\n");
		cons->set_color();

		cons->printf("         ISR: %02lx\n", regs.interrupt);
		cons->printf("         ERR: %lx\n", regs.errorcode);
		cons->puts("\n");

		print_regs(regs);
		print_stacktrace(2);
		_halt();
	}
	*reinterpret_cast<uint32_t *>(0xffffff80fee000b0) = 0;

	return return_rsp;
}

addr_t
irq_handler(addr_t return_rsp, cpu_state regs)
{
	console *cons = console::get();
	uint8_t irq = get_irq(regs.interrupt);
	if (! device_manager::get().dispatch_irq(irq)) {
		cons->set_color(11);
		cons->printf("\nReceived unknown IRQ: %d (vec %d)\n",
				irq, regs.interrupt);
		cons->set_color();
		print_regs(regs);
		_halt();
	}

	*reinterpret_cast<uint32_t *>(0xffffff80fee000b0) = 0;
	return return_rsp;
}

addr_t
syscall_handler(addr_t return_rsp, cpu_state regs)
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

	default:
		cons->set_color(9);
		cons->printf("\nReceived unknown syscall: %02x\n", call);
		cons->set_color();
		print_regs(regs);
		_halt();
		break;
	}

	return return_rsp;
}
