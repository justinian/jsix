#pragma once
/// \file apic.h
/// Classes to control both local and I/O APICs.

#include <stdint.h>

enum class isr : uint8_t;


/// Base class for other APIC types
class apic
{
public:
	/// Constructor
	/// \arg base   Base virtual address of the APIC's MMIO registers
	apic(uint32_t *base);

protected:
	uint32_t *m_base;
};


/// Controller for processor-local APICs
class lapic :
	public apic
{
public:
	/// Constructor
	/// \arg base      Base virtual address of the APIC's MMIO registers
	/// \arg spurious  Vector of the spurious interrupt handler
	lapic(uint32_t *base, isr spurious);

	/// Enable interrupts for the LAPIC timer.
	/// \arg vector   Interrupt vector the timer should use
	/// \arg divisor  The frequency divisor of the bus Hz (power of 2, <= 128)
	/// \arg count    The count of ticks before an interrupt
	/// \arg repeat   If false, this timer is one-off, otherwise repeating
	void enable_timer(isr vector, uint8_t divisor, uint32_t count, bool repeat = true);

	/// Enable interrupts for the LAPIC LINT0 pin.
	/// \arg num      Local interrupt number (0 or 1)
	/// \arg vector   Interrupt vector LINT0 should use
	/// \arg nmi      Whether this interrupt is NMI delivery mode
	/// \arg flags    Flags for mode/polarity (ACPI MPS INTI flags)
	void enable_lint(uint8_t num, isr vector, bool nmi, uint16_t flags);

	void enable();  ///< Enable servicing of interrupts
	void disable(); ///< Disable (temporarily) servicing of interrupts
};


/// Controller for I/O APICs
class ioapic :
	public apic
{
public:
	/// Constructor
	/// \arg base     Base virtual address of the APIC's MMIO registers
	/// \arg base_gsr Starting global system interrupt number of this IOAPIC
	ioapic(uint32_t *base, uint32_t base_gsr);

	uint32_t get_base_gsr() const	{ return m_base_gsr; }
	uint32_t get_num_gsr() const	{ return m_num_gsr; }

private:
	uint32_t m_base_gsr;
	uint32_t m_num_gsr;
};

