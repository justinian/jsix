#include "apic.h"
#include "assert.h"
#include "interrupts.h"
#include "log.h"
#include "memory_pages.h"


static uint32_t
apic_read(uint32_t *apic, uint16_t offset)
{
	return *(apic + offset/sizeof(uint32_t));
}

static void
apic_write(uint32_t *apic, uint16_t offset, uint32_t value)
{
	*(apic + offset/sizeof(uint32_t)) = value;
}


apic::apic(uint32_t *base) :
	m_base(base)
{
	g_page_manager.map_offset_pointer(reinterpret_cast<void **>(&m_base), 0x1000);
}


lapic::lapic(uint32_t *base, isr spurious) :
	apic(base)
{
	apic_write(m_base, 0xf0, static_cast<uint32_t>(spurious));
	log::info(logs::interrupt, "LAPIC created, base %lx", m_base);
}

void
lapic::enable_timer(isr vector, uint8_t divisor, uint32_t count, bool repeat)
{
	switch (divisor) {
	case   1: divisor = 11; break;
	case   2: divisor = 0; break;
	case   4: divisor = 1; break;
	case   8: divisor = 2; break;
	case  16: divisor = 3; break;
	case  32: divisor = 8; break;
	case  64: divisor = 9; break;
	case 128: divisor = 10; break;
	default:
		kassert(0, "Invalid divisor passed to lapic::enable_timer");
	}

	apic_write(m_base, 0x3e0, divisor);
	apic_write(m_base, 0x380, count);

	uint32_t lvte = static_cast<uint8_t>(vector);
	if (repeat)
		lvte |= 0x20000;

	log::debug(logs::interrupt, "Enabling APIC timer with isr %d.", vector);
	apic_write(m_base, 0x320, lvte);
}

void
lapic::enable_lint(uint8_t num, isr vector, bool nmi, uint16_t flags)
{
	kassert(num == 0 || num == 1, "Invalid LINT passed to lapic::enable_lint.");

	uint16_t off = num ? 0x360 : 0x350;
	uint32_t lvte = static_cast<uint8_t>(vector);

	uint16_t polarity = flags & 0x3;
	if (polarity == 3)
		lvte |=	(1 << 13);

	uint16_t trigger = (flags >> 2) & 0x3;
	if (trigger == 3)
		lvte |= (1 << 15);

	apic_write(m_base, off, lvte);
	log::debug(logs::interrupt, "APIC LINT%d enabled as %s %d %s-triggered, active %s.",
			num, nmi ? "NMI" : "ISR", vector,
			polarity == 3 ? "level" : "edge",
			trigger == 3 ? "low" : "high");
}

void
lapic::enable()
{
	apic_write(m_base, 0xf0,
			apic_read(m_base, 0xf0) | 0x100);
	log::debug(logs::interrupt, "LAPIC enabled!");
}

void
lapic::disable()
{
	apic_write(m_base, 0xf0,
			apic_read(m_base, 0xf0) & ~0x100);
	log::debug(logs::interrupt, "LAPIC disabled.");
}


ioapic::ioapic(uint32_t *base, uint32_t base_gsr) :
	apic(base),
	m_base_gsr(base_gsr)
{
}
