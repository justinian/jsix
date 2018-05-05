#include <stddef.h>
#include <stdint.h>

#include "kutil/enum_bitfields.h"
#include "kutil/memory.h"
#include "console.h"
#include "interrupts.h"
#include "io.h"
#include "log.h"
#include "memory_pages.h"

enum class gdt_flags : uint8_t
{
	ac = 0x01,
	rw = 0x02,
	dc = 0x04,
	ex = 0x08,
	r1 = 0x20,
	r2 = 0x40,
	r3 = 0x60,
	pr = 0x80,

	res_1 = 0x10
};
IS_BITFIELD(gdt_flags);

struct gdt_descriptor
{
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t flags;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__ ((packed));

struct idt_descriptor
{
	uint16_t base_low;
	uint16_t selector;
	uint8_t ist;
	uint8_t flags;
	uint16_t base_mid;
	uint32_t base_high;
	uint32_t reserved;		// must be zero
} __attribute__ ((packed));

struct table_ptr
{
	uint16_t limit;
	uint64_t base;
} __attribute__ ((packed));

gdt_descriptor g_gdt_table[10];
idt_descriptor g_idt_table[256];
table_ptr g_gdtr;
table_ptr g_idtr;

struct registers;

extern "C" {
	void idt_write();
	void idt_load();

	void gdt_write();
	void gdt_load();

	void isr_handler(registers);
	void irq_handler(registers);

#define ISR(i, name)     extern void name ();
#define EISR(i, name)    extern void name ();
#define IRQ(i, q, name)  extern void name ();
#include "interrupt_isrs.inc"
#undef IRQ
#undef EISR
#undef ISR
}

void idt_dump(const table_ptr &table);
void gdt_dump(const table_ptr &table);

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

void
set_gdt_entry(uint8_t i, uint32_t base, uint32_t limit, bool is64, gdt_flags flags)
{
	g_gdt_table[i].limit_low = limit & 0xffff;

	g_gdt_table[i].base_low = base & 0xffff;
	g_gdt_table[i].base_mid = (base >> 16) & 0xff;
	g_gdt_table[i].base_high = (base >> 24) & 0xff;

	g_gdt_table[i].granularity =
			((limit >> 16) & 0xf) | (is64 ? 0xa0 : 0xc0);

	g_gdt_table[i].flags = static_cast<uint8_t>(flags | gdt_flags::res_1 | gdt_flags::pr);
}

void
set_idt_entry(uint8_t i, uint64_t addr, uint16_t selector, uint8_t flags)
{
	g_idt_table[i].base_low = addr & 0xffff;
	g_idt_table[i].base_mid = (addr >> 16) & 0xffff;
	g_idt_table[i].base_high = (addr >> 32) & 0xffffffff;
	g_idt_table[i].selector = selector;
	g_idt_table[i].flags = flags;
	g_idt_table[i].ist = 0;
	g_idt_table[i].reserved = 0;
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
	outb(PIC2+1, static_cast<uint8_t>(isr::isrIgnore8)); io_wait();

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
	kutil::memset(&g_gdt_table, 0, sizeof(g_gdt_table));
	kutil::memset(&g_idt_table, 0, sizeof(g_idt_table));

	g_gdtr.limit = sizeof(g_gdt_table) - 1;
	g_gdtr.base = reinterpret_cast<uint64_t>(&g_gdt_table);

	set_gdt_entry(1, 0, 0xfffff, false, gdt_flags::rw);
	set_gdt_entry(2, 0, 0xfffff, false, gdt_flags::rw | gdt_flags::ex | gdt_flags::dc);
	set_gdt_entry(3, 0, 0xfffff, false, gdt_flags::rw);
	set_gdt_entry(4, 0, 0xfffff, false, gdt_flags::rw | gdt_flags::ex);

	set_gdt_entry(6, 0, 0xfffff, false, gdt_flags::rw);
	set_gdt_entry(7, 0, 0xfffff,  true, gdt_flags::rw | gdt_flags::ex);

	gdt_write();

	g_idtr.limit = sizeof(g_idt_table) - 1;
	g_idtr.base = reinterpret_cast<uint64_t>(&g_idt_table);

#define ISR(i, name)     set_idt_entry(i, reinterpret_cast<uint64_t>(& name), 0x38, 0x8e);
#define EISR(i, name)    set_idt_entry(i, reinterpret_cast<uint64_t>(& name), 0x38, 0x8e);
#define IRQ(i, q, name)  set_idt_entry(i, reinterpret_cast<uint64_t>(& name), 0x38, 0x8e);
#include "interrupt_isrs.inc"
#undef IRQ
#undef EISR
#undef ISR

	idt_write();
	disable_legacy_pic();
	enable_serial_interrupts();

	log::info(logs::boot, "Interrupts enabled.");
}

struct registers
{
	uint64_t ds;
	uint64_t rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;
	uint64_t interrupt, errorcode;
	uint64_t rip, cs, eflags, user_esp, ss;
};

#define print_reg(name, value) cons->printf("         %s: %lx\n", name, (value));

void
isr_handler(registers regs)
{
	console *cons = console::get();

	switch (static_cast<isr>(regs.interrupt & 0xff)) {
	case isr::isrTimer:
		cons->puts("\nTICK\n");
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
	case isr::isrIgnore8:
	case isr::isrIgnore9:
	case isr::isrIgnoreA:
	case isr::isrIgnoreB:
	case isr::isrIgnoreC:
	case isr::isrIgnoreD:
	case isr::isrIgnoreE:
	case isr::isrIgnoreF:
		/*
		cons->printf("\nIGNORED PIC INTERRUPT %d\n",
				(regs.interrupt % 0xff) - 0xf0);
		*/
		break;

	default:
		cons->set_color(9);
		cons->puts("\nReceived ISR interrupt:\n");
		cons->set_color();

		uint64_t cr2 = 0;
		__asm__ __volatile__ ("mov %%cr2, %0" : "=r"(cr2));

		print_reg("ISR", regs.interrupt);
		print_reg("ERR", regs.errorcode);
		print_reg("CR2", cr2);
		cons->puts("\n");

		print_reg(" ds", regs.ds);
		print_reg("rdi", regs.rdi);
		print_reg("rsi", regs.rsi);
		print_reg("rbp", regs.rbp);
		print_reg("rsp", regs.rsp);
		print_reg("rbx", regs.rbx);
		print_reg("rdx", regs.rdx);
		print_reg("rcx", regs.rcx);
		print_reg("rax", regs.rax);
		cons->puts("\n");

		print_reg("rip", regs.rip);
		print_reg(" cs", regs.cs);
		print_reg(" ef", regs.eflags);
		print_reg("esp", regs.user_esp);
		print_reg(" ss", regs.ss);
		while(1) asm("hlt");
	}

	*reinterpret_cast<uint32_t *>(0xffffff80fee000b0) = 0;
}

void
irq_handler(registers regs)
{
	console *cons = console::get();
	uint8_t irq = get_irq(regs.interrupt);

	switch (irq) {
	case 2:
		cons->set_color(11);
		cons->puts(".");
		cons->set_color();
		break;

	case 4:
		// TODO: move this to a real serial driver
		cons->echo();
		break;

	default:
		cons->set_color(11);
		cons->printf("\nReceived IRQ interrupt: %d (vec %d)\n",
				irq, regs.interrupt);
		cons->set_color();

		print_reg(" ds", regs.ds);
		print_reg("rdi", regs.rdi);
		print_reg("rsi", regs.rsi);
		print_reg("rbp", regs.rbp);
		print_reg("rsp", regs.rsp);
		print_reg("rbx", regs.rbx);
		print_reg("rdx", regs.rdx);
		print_reg("rcx", regs.rcx);
		print_reg("rax", regs.rax);
		cons->puts("\n");

		print_reg("rip", regs.rip);
		print_reg(" cs", regs.cs);
		print_reg(" ef", regs.eflags);
		print_reg("esp", regs.user_esp);
		print_reg(" ss", regs.ss);
		while(1) asm("hlt");
	}

	*reinterpret_cast<uint32_t *>(0xffffff80fee000b0) = 0;
}


void
gdt_dump(const table_ptr &table)
{
	log::info(logs::boot, "Loaded GDT at: %lx size: %d bytes", table.base, table.limit+1);

	int count = (table.limit + 1) / sizeof(gdt_descriptor);
	const gdt_descriptor *gdt =
		reinterpret_cast<const gdt_descriptor *>(table.base);

	for (int i = 0; i < count; ++i) {
		uint32_t base =
			(gdt[i].base_high << 24) |
			(gdt[i].base_mid << 16) |
			gdt[i].base_low;

		uint32_t limit =
			static_cast<uint32_t>(gdt[i].granularity & 0x0f) << 16 |
			gdt[i].limit_low;

		if (gdt[i].flags & 0x80) {
			log::debug(logs::boot,
					"   Entry %3d: Base %x  limit %x  privs %d  flags %s%s%s%s%s%s",
					i, base, limit, ((gdt[i].flags >> 5) & 0x03),

					(gdt[i].flags & 0x80) ? "P " : "  ",
					(gdt[i].flags & 0x08) ? "ex " : "   ",
					(gdt[i].flags & 0x04) ? "dc " : "   ",
					(gdt[i].flags & 0x02) ? "rw " : "   ",

					(gdt[i].granularity & 0x80) ? "kb " : "b  ",
					(gdt[i].granularity & 0x60) == 0x60 ? "64" :
						(gdt[i].granularity & 0x60) == 0x40 ? "32" : "16"
					);
		}
	}
}

void
idt_dump(const table_ptr &table)
{
	log::info(logs::boot, "Loaded IDT at: %lx size: %d bytes", table.base, table.limit+1);

	int count = (table.limit + 1) / sizeof(idt_descriptor);
	const idt_descriptor *idt =
		reinterpret_cast<const idt_descriptor *>(table.base);

	for (int i = 0; i < count; ++i) {
		uint64_t base =
			(static_cast<uint64_t>(idt[i].base_high) << 32) |
			(static_cast<uint64_t>(idt[i].base_mid)  << 16) |
			idt[i].base_low;

		char const *type;
		switch (idt[i].flags & 0xf) {
			case 0x5: type = " 32tsk "; break;
			case 0x6: type = " 16int "; break;
			case 0x7: type = " 16trp "; break;
			case 0xe: type = " 32int "; break;
			case 0xf: type = " 32trp "; break;
			default:  type = " ????? "; break;
		}

		if (idt[i].flags & 0x80) {
			log::debug(logs::boot,
					"   Entry %3d: Base:%lx Sel(rpl %d, ti %d, %3d) IST:%d %s DPL:%d", i, base,
					(idt[i].selector & 0x3),
					((idt[i].selector & 0x4) >> 2),
					(idt[i].selector >> 3),
					idt[i].ist,
					type,
					((idt[i].flags >> 5) & 0x3));
		}
	}
}
