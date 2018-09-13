#pragma once
/// \file interrupts.h
/// Free functions and definitions related to interrupt service vectors
#include <stdint.h>


/// Enum of all defined ISR/IRQ vectors
enum class isr : uint8_t
{
#define ISR(i, name)     name = i,
#define EISR(i, name)    name = i,
#define UISR(i, name)    name = i,
#define IRQ(i, q, name)  name = i,
#include "interrupt_isrs.inc"
#undef IRQ
#undef UISR
#undef EISR
#undef ISR

	_zero = 0
};

/// Helper operator to add an offset to an isr vector
isr operator+(const isr &lhs, int rhs);

extern "C" {
	/// Set the CPU interrupt enable flag (sti)
	void interrupts_enable();

	/// Set the CPU interrupt disable flag (cli)
	void interrupts_disable();
}

/// Fill the IDT with our ISRs, and disable the legacy
/// PIC interrupts.
void interrupts_init();
