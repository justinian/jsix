#include <stdint.h>

#include "kernel_memory.h"
#include "kutil/printf.h"

#include "apic.h"
#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "device_manager.h"
#include "gdt.h"
#include "idt.h"
#include "interrupts.h"
#include "io.h"
#include "kernel_memory.h"
#include "log.h"
#include "objects/process.h"
#include "scheduler.h"
#include "syscall.h"
#include "tss.h"
#include "vm_space.h"

static const uint16_t PIC1 = 0x20;
static const uint16_t PIC2 = 0xa0;

constexpr uintptr_t apic_eoi_addr = 0xfee000b0 + ::memory::page_offset;

constexpr size_t increment_offset = 0x1000;

extern "C" {
	void _halt();

	void isr_handler(cpu_state*);
	void irq_handler(cpu_state*);

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
#define NISR(i, s, name)
#define IRQ(i, q, name) case i : return q;
#include "interrupt_isrs.inc"
#undef IRQ
#undef NISR
#undef EISR
#undef ISR

		default: return 0xff;
	}
}

void
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
isr_handler(cpu_state *regs)
{
	uint8_t vector = regs->interrupt & 0xff;
    if ((vector & 0xf0) == 0xf0) {
        *reinterpret_cast<uint32_t *>(apic_eoi_addr) = 0;
        return;
    }

	// Clear out the IST for this vector so we just keep using
	// this stack
	IDT &idt = IDT::current();
	uint8_t old_ist = idt.get_ist(vector);
	if (old_ist)
		idt.set_ist(vector, 0);

    char message[200];

	switch (static_cast<isr>(vector)) {

	case isr::isrDebug:
        asm volatile ("mov %%dr0, %%r8" ::: "r8");
        asm volatile ("mov %%dr1, %%r9" ::: "r9");
        asm volatile ("mov %%dr2, %%r10" ::: "r10");
        asm volatile ("mov %%dr3, %%r11" ::: "r11");
        asm volatile ("mov %%dr4, %%r12" ::: "r12");
        asm volatile ("mov %%dr5, %%r13" ::: "r13");
        asm volatile ("mov %%dr6, %%r14" ::: "r14");
        asm volatile ("mov %%dr7, %%r15" ::: "r15");
        kassert(false, "Debug exception");
		break;

	case isr::isrDoubleFault:
        kassert(false, "Double fault");
		break;

	case isr::isrGPFault:
        if (regs->errorcode & 0xfff0) {
			int index = (regs->errorcode & 0xffff) >> 4;
            int ti = (regs->errorcode & 0x07) >> 1;
            char const *table =
                (ti & 1) ? "IDT" :
                (!ti) ? "GDT" :
                "LDT";

            snprintf(message, sizeof(message), "General Protection Fault, error:%lx%s %s[%d]",
                regs->errorcode, regs->errorcode & 1 ? " external" : "", table, index);
        } else {
            snprintf(message, sizeof(message), "General Protection Fault, error:%lx%s",
                regs->errorcode, regs->errorcode & 1 ? " external" : "");
        }
        kassert(false, message);
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

            snprintf(message, sizeof(message),
                "Page fault: %016llx%s%s%s%s%s", cr2,
                (regs->errorcode & 0x01) ? " present" : "",
                (regs->errorcode & 0x02) ? " write" : "",
                (regs->errorcode & 0x04) ? " user" : "",
                (regs->errorcode & 0x08) ? " reserved" : "",
                (regs->errorcode & 0x10) ? " ip" : "");
            kassert(false, message);
		}
		break;

	case isr::isrTimer:
		scheduler::get().schedule();
		break;

	case isr::isrLINT0:
	case isr::isrLINT1:
		break;

	case isr::isrSpurious:
		// No EOI for the spurious interrupt
		return;

	default:
        snprintf(message, sizeof(message), "Unknown interrupt 0x%x", regs->interrupt);
        kassert(false, message);
	}

	// Return the IST for this vector to what it was
	if (old_ist)
		idt.set_ist(vector, old_ist);
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
