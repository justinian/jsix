#include "apic.h"
#include "assert.h"
#include "interrupts.h"
#include "log.h"
#include "memory_pages.h"


static uint32_t
apic_read(uint32_t volatile *apic, uint16_t offset)
{
	return *(apic + offset/sizeof(uint32_t));
}

static void
apic_write(uint32_t volatile *apic, uint16_t offset, uint32_t value)
{
	*(apic + offset/sizeof(uint32_t)) = value;
}

static uint32_t
ioapic_read(uint32_t volatile *base, uint8_t reg)
{
	*base = reg;
	return *(base + 4);
}

static void
ioapic_write(uint32_t volatile *base, uint8_t reg, uint32_t value)
{
	*base = reg;
	*(base + 4) = value;
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
	log::info(logs::apic, "LAPIC created, base %lx", m_base);
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

	log::debug(logs::apic, "Enabling APIC timer with isr %d.", vector);
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
		lvte |= (1 << 13);

	uint16_t trigger = (flags >> 2) & 0x3;
	if (trigger == 3)
		lvte |= (1 << 15);

	apic_write(m_base, off, lvte);
	log::debug(logs::apic, "APIC LINT%d enabled as %s %d %s-triggered, active %s.",
			num, nmi ? "NMI" : "ISR", vector,
			polarity == 3 ? "level" : "edge",
			trigger == 3 ? "low" : "high");
}

void
lapic::enable()
{
	apic_write(m_base, 0xf0,
			apic_read(m_base, 0xf0) | 0x100);
	log::debug(logs::apic, "LAPIC enabled!");
}

void
lapic::disable()
{
	apic_write(m_base, 0xf0,
			apic_read(m_base, 0xf0) & ~0x100);
	log::debug(logs::apic, "LAPIC disabled.");
}


ioapic::ioapic(uint32_t *base, uint32_t base_gsi) :
	apic(base),
	m_base_gsi(base_gsi)
{
	uint32_t id = ioapic_read(m_base, 0);
	uint32_t version = ioapic_read(m_base, 1);

	m_id = (id >> 24) & 0xff;
	m_version = version & 0xff;
	m_num_gsi = (version >> 16) & 0xff;
	log::debug(logs::apic, "IOAPIC %d loaded, version %d, GSIs %d-%d",
			m_id, m_version, base_gsi, base_gsi + (m_num_gsi - 1));

	for (uint8_t i = 0; i < m_num_gsi; ++i) {
		uint16_t flags = (i < 0x10) ? 0 : 0xf;
		isr vector = isr::irq0 + i;
		redirect(i, vector, flags, true);
	}
}

void
ioapic::redirect(uint8_t irq, isr vector, uint16_t flags, bool masked)
{
	uint64_t entry = static_cast<uint64_t>(vector);

	uint16_t polarity = flags & 0x3;
	if (polarity == 3)
		entry |= (1 << 13);

	uint16_t trigger = (flags >> 2) & 0x3;
	if (trigger == 3)
		entry |= (1 << 15);

	if (masked)
		entry |= (1 << 16);

	ioapic_write(m_base, (2 * irq) + 0x10, static_cast<uint32_t>(entry & 0xffffffff));
	ioapic_write(m_base, (2 * irq) + 0x11, static_cast<uint32_t>(entry >> 32));
}

void
ioapic::mask(uint8_t irq, bool masked)
{
	uint32_t entry = ioapic_read(m_base, (2 * irq) + 0x10);
	if (masked)
		entry |= (1 << 16);
	else
		entry &= ~(1 << 16);

	ioapic_write(m_base, (2 * irq) + 0x10, entry);
}

void
ioapic::dump_redirs() const
{
	log::debug(logs::apic, "IOAPIC %d redirections:", m_id);

	for (uint8_t i = 0; i < m_num_gsi; ++i) {
		uint64_t low = ioapic_read(m_base, 0x10 + (2 *i));
		uint64_t high = ioapic_read(m_base, 0x10 + (2 *i));
		uint64_t redir = low | (high << 32);
		if (redir == 0) continue;

		uint8_t vector = redir & 0xff;
		uint8_t mode = (redir >> 8) & 0x7;
		uint8_t dest_mode = (redir >> 11) & 0x1;
		uint8_t polarity = (redir >> 13) & 0x1;
		uint8_t trigger = (redir >> 15) & 0x1;
		uint8_t mask = (redir >> 16) & 0x1;
		uint8_t dest = (redir >> 56) & 0xff;

		log::debug(logs::apic, "  %2d: vec %3d %s active, %s-triggered %s dest %d: %x",
				m_base_gsi + i, vector,
				polarity ? "low" : "high",
				trigger ? "level" : "edge",
				mask ? "masked" : "",
				dest_mode,
				dest);
	}
}
