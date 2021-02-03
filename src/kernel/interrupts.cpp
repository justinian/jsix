#include <stdint.h>

#include "kernel_memory.h"

#include "apic.h"
#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "device_manager.h"
#include "gdt.h"
#include "interrupts.h"
#include "io.h"
#include "kernel_memory.h"
#include "log.h"
#include "objects/process.h"
#include "scheduler.h"
#include "syscall.h"
#include "vm_space.h"

static const uint16_t PIC1 = 0x20;
static const uint16_t PIC2 = 0xa0;

constexpr uintptr_t apic_eoi_addr = 0xfee000b0 + ::memory::page_offset;

extern "C" {
	void _halt();

	void isr_handler(cpu_state*);
	void irq_handler(cpu_state*);

#define ISR(i, s, name)     extern void name ();
#define EISR(i, s, name)    extern void name ();
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
#define ISR(i, s, name)
#define EISR(i, s, name)
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
	// Mask all interrupts
	outb(PIC2+1, 0xfc);
	outb(PIC1+1, 0xff);

	// Start initialization sequence
	outb(PIC1, 0x11); io_wait();
	outb(PIC2, 0x11); io_wait();

	// Remap into ignore ISRs
	outb(PIC1+1, static_cast<uint8_t>(isr::isrIgnore0)); io_wait();
	outb(PIC2+1, static_cast<uint8_t>(isr::isrIgnore8)); io_wait();

	// Tell PICs about each other
	outb(PIC1+1, 0x04); io_wait();
	outb(PIC2+1, 0x02); io_wait();
}

void
interrupts_init()
{
#define ISR(i, s, name)     idt_set_entry(i, reinterpret_cast<uint64_t>(& name), 0x08, 0x8e);
#define EISR(i, s, name)    idt_set_entry(i, reinterpret_cast<uint64_t>(& name), 0x08, 0x8e);
#define IRQ(i, q, name)  idt_set_entry(i, reinterpret_cast<uint64_t>(& name), 0x08, 0x8e);
#include "interrupt_isrs.inc"
#undef IRQ
#undef EISR
#undef ISR

	disable_legacy_pic();

	log::info(logs::boot, "Interrupts enabled.");
}

void
isr_handler(cpu_state *regs)
{
	console *cons = console::get();
	uint8_t vector = regs->interrupt & 0xff;
	ist_decrement(vector);

	switch (static_cast<isr>(vector)) {

	case isr::isrDebug: {
			cons->set_color(11);
			cons->puts("\nDebug Exception:\n");
			cons->set_color();

			uint64_t dr = 0;

			__asm__ __volatile__ ("mov %%dr0, %0" : "=r"(dr));
			print_regL("dr0", dr);

			__asm__ __volatile__ ("mov %%dr1, %0" : "=r"(dr));
			print_regM("dr1", dr);

			__asm__ __volatile__ ("mov %%dr2, %0" : "=r"(dr));
			print_regM("dr2", dr);

			__asm__ __volatile__ ("mov %%dr3, %0" : "=r"(dr));
			print_regR("dr3", dr);

			__asm__ __volatile__ ("mov %%dr6, %0" : "=r"(dr));
			print_regL("dr6", dr);

			__asm__ __volatile__ ("mov %%dr7, %0" : "=r"(dr));
			print_regR("dr7", dr);

			print_regL("rip", regs->rip);
			print_regM("rsp", regs->user_rsp);
			print_regM("fla", regs->rflags);
			_halt();
		}
		break;

	case isr::isrGPFault: {
			cons->set_color(9);
			cons->puts("\nGeneral Protection Fault:\n");
			cons->set_color();

			cons->printf("   errorcode: %lx", regs->errorcode);
			if (regs->errorcode & 0x01) cons->puts(" external");

			int index = (regs->errorcode & 0xffff) >> 4;
			if (index) {
				switch ((regs->errorcode & 0x07) >> 1) {
				case 0:
					cons->printf(" GDT[%x]\n", index);
					gdt_dump(index);
					break;

				case 1:
				case 3:
					cons->printf(" IDT[%x]\n", index);
					idt_dump(index);
					break;

				default:
					cons->printf(" LDT[%x]??\n", index);
					break;
				}
			} else {
				cons->putc('\n');
			}
			print_regs(*regs);
			/*
			print_stacktrace(2);
			print_stack(*regs);
			*/
		}
		_halt();
		break;

	case isr::isrPageFault: {
			uintptr_t cr2 = 0;
			__asm__ __volatile__ ("mov %%cr2, %0" : "=r"(cr2));

			bool user = cr2 < memory::kernel_offset;
			vm_space::fault_type ft =
				static_cast<vm_space::fault_type>(regs->errorcode);

			vm_space &space = user
				? process::current().space()
				: vm_space::kernel_space();

			if (cr2 && space.handle_fault(cr2, ft))
				break;

			cons->set_color(11);
			cons->puts("\nPage Fault:\n");
			cons->set_color();

			cons->puts("       flags:");
			if (regs->errorcode & 0x01) cons->puts(" present");
			if (regs->errorcode & 0x02) cons->puts(" write");
			if (regs->errorcode & 0x04) cons->puts(" user");
			if (regs->errorcode & 0x08) cons->puts(" reserved");
			if (regs->errorcode & 0x10) cons->puts(" ip");
			cons->puts("\n");
			print_regs(*regs);
			print_stacktrace(2);
			_halt();
		}
		break;

	case isr::isrTimer:
		scheduler::get().schedule();
		break;

	case isr::isrLINT0:
		cons->puts("\nLINT0\n");
		break;

	case isr::isrLINT1:
		cons->puts("\nLINT1\n");
		break;

	case isr::isrAssert: {
			cons->set_color();
			print_regs(*regs);
			print_stacktrace(2);
		}
		_halt();
		break;

	/*
	case isr::isrSyscall:
		syscall_dispatch(regs);
		break;
	*/

	case isr::isrSpurious:
		// No EOI for the spurious interrupt
		return;

	case isr::isrIgnore0:
	case isr::isrIgnore1:
	case isr::isrIgnore2:
	case isr::isrIgnore3:
	case isr::isrIgnore4:
	case isr::isrIgnore5:
	case isr::isrIgnore6:
	case isr::isrIgnore7:
		//cons->printf("\nIGNORED: %02x\n", regs->interrupt);
		outb(PIC1, 0x20);
		break;

	case isr::isrIgnore8:
	case isr::isrIgnore9:
	case isr::isrIgnoreA:
	case isr::isrIgnoreB:
	case isr::isrIgnoreC:
	case isr::isrIgnoreD:
	case isr::isrIgnoreE:
	case isr::isrIgnoreF:
		//cons->printf("\nIGNORED: %02x\n", regs->interrupt);
		outb(PIC1, 0x20);
		outb(PIC2, 0x20);
		break;

	default:
		cons->set_color(9);
		cons->printf("\nReceived %02x interrupt:\n",
				(static_cast<isr>(regs->interrupt)));

		cons->set_color();
		cons->printf("         ISR: %02lx  ERR: %lx\n\n",
				regs->interrupt, regs->errorcode);

		print_regs(*regs);
		print_stacktrace(2);
		_halt();
	}
	ist_increment(vector);
	*reinterpret_cast<uint32_t *>(apic_eoi_addr) = 0;
}

void
irq_handler(cpu_state *regs)
{
	console *cons = console::get();
	uint8_t irq = get_irq(regs->interrupt);
	if (! device_manager::get().dispatch_irq(irq)) {
		cons->set_color(11);
		cons->printf("\nReceived unknown IRQ: %d (vec %d)\n",
				irq, regs->interrupt);
		cons->set_color();

		print_regs(*regs);
	}

	*reinterpret_cast<uint32_t *>(apic_eoi_addr) = 0;
}
