#include "kutil/assert.h"
#include "apic.h"
#include "clock.h"
#include "interrupts.h"
#include "io.h"
#include "log.h"
#include "page_manager.h"

static constexpr uint16_t lapic_spurious   = 0x00f0;

static constexpr uint16_t lapic_lvt_timer  = 0x0320;
static constexpr uint16_t lapic_lvt_lint0  = 0x0350;
static constexpr uint16_t lapic_lvt_lint1  = 0x0360;

static constexpr uint16_t lapic_timer_init = 0x0380;
static constexpr uint16_t lapic_timer_cur  = 0x0390;
static constexpr uint16_t lapic_timer_div  = 0x03e0;

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
	// Map 1MiB of space for the APIC registers and
	// MSI area
	page_manager::get()->map_offset_pointer(
			reinterpret_cast<void **>(&m_base),
			0x100000);
}


lapic::lapic(uint32_t *base, isr spurious) :
	apic(base),
	m_divisor(0)
{
	apic_write(m_base, lapic_spurious, static_cast<uint32_t>(spurious));
	log::info(logs::apic, "LAPIC created, base %lx", m_base);
}

void
lapic::calibrate_timer()
{
	interrupts_disable();

	log::info(logs::apic, "Calibrating APIC timer...");

	const uint32_t initial = -1u;
	enable_timer(isr::isrSpurious);
	set_divisor(1);
	apic_write(m_base, lapic_timer_init, initial);

	uint64_t us = 200000;
	uint64_t ns = us * 1000;
	clock::get().spinwait(ns);

	uint32_t remaining = apic_read(m_base, lapic_timer_cur);
	uint32_t ticks_total = initial - remaining;
	m_ticks_per_us = ticks_total / us;

	log::info(logs::apic, "APIC timer ticks %d times per microsecond.", m_ticks_per_us);

	interrupts_enable();
}

void
lapic::set_divisor(uint8_t divisor)
{
	uint32_t divbits = 0;

	switch (divisor) {
	case   1: divbits = 0xb; break;
	case   2: divbits = 0x0; break;
	case   4: divbits = 0x1; break;
	case   8: divbits = 0x2; break;
	case  16: divbits = 0x3; break;
	case  32: divbits = 0x8; break;
	case  64: divbits = 0x9; break;
	case 128: divbits = 0xa; break;
	default:
		kassert(0, "Invalid divisor passed to lapic::enable_timer");
	}

	apic_write(m_base, lapic_timer_div, divbits);
	m_divisor = divisor;
}

void
lapic::enable_timer(isr vector, bool repeat)
{
	uint32_t lvte = static_cast<uint8_t>(vector);
	if (repeat)
		lvte |= 0x20000;
	apic_write(m_base, lapic_lvt_timer, lvte);

	log::debug(logs::apic, "Enabling APIC timer at isr %02x", vector);
}

uint32_t
lapic::reset_timer(uint64_t interval)
{
	uint64_t remaining = ticks_to_us(apic_read(m_base, lapic_timer_cur));
	uint64_t ticks = us_to_ticks(interval);

	int divisor = 1;
	while (ticks > 0xffffffffull) {
		ticks >>= 1;
		divisor <<= 1;
	}

	if (divisor != m_divisor)
		set_divisor(divisor);

	apic_write(m_base, lapic_timer_init, ticks);
	return remaining;
}

void
lapic::enable_lint(uint8_t num, isr vector, bool nmi, uint16_t flags)
{
	kassert(num == 0 || num == 1, "Invalid LINT passed to lapic::enable_lint.");

	uint16_t off = num ? lapic_lvt_lint1 : lapic_lvt_lint0;
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
	apic_write(m_base, lapic_spurious,
			apic_read(m_base, lapic_spurious) | 0x100);
	log::debug(logs::apic, "LAPIC enabled!");
}

void
lapic::disable()
{
	apic_write(m_base, lapic_spurious,
			apic_read(m_base, lapic_spurious) & ~0x100);
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
		isr vector = isr::irq00 + i;
		redirect(i, vector, flags, true);
	}
}

void
ioapic::redirect(uint8_t irq, isr vector, uint16_t flags, bool masked)
{
	log::debug(logs::apic, "IOAPIC %d redirecting irq %3d to vector %3d [%04x]%s",
			m_id, irq, vector, flags, masked ? " (masked)" : "");

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
	log::debug(logs::apic, "IOAPIC %d %smasking irq %3d",
			m_id, masked ? "" : "un", irq);

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
