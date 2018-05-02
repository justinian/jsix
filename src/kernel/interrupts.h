#pragma once
/// \file interrupts.h
/// Free functions and definitions related to interrupt service vectors


/// Enum of all defined ISR/IRQ vectors
enum class isr : uint8_t
{
#define ISR(i, name)     name = i,
#define EISR(i, name)    name = i,
#define IRQ(i, q, name)  name = i,
#include "interrupt_isrs.inc"
#undef IRQ
#undef EISR
#undef ISR

	_zero = 0
};

isr operator+(const isr &lhs, int rhs);

extern "C" {
	void interrupts_enable();
	void interrupts_disable();
}

void interrupts_init();
