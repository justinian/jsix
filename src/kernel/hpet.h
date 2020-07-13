#pragma once
/// \file hpet.h
/// Definitions for handling the HPET timer

#include <stdint.h>

/// Source function for the clock that takes a pointer to
/// an hpet object.
uint64_t hpet_clock_source(void *);

/// Represents a single HPET timer
class hpet
{
public:
	/// Constructor.
	/// \arg index  The index of the HPET out of all HPETs
	/// \arg base   The base address of the HPET for MMIO
	hpet(uint8_t index, uint64_t *base);

	/// Configure the timer and start it running.
	void enable();

	/// Get the timer rate in ticks per us
	inline uint64_t rate() const { return 1000000000/m_period; }

	/// Get the current timer value
	uint64_t value() const;

private:
	friend void hpet_irq_callback(void *);
	friend uint64_t hpet_clock_source(void *);

	void setup_timer_interrupts(unsigned timer, uint8_t irq, uint64_t interval, bool periodic);
	void enable_timer(unsigned timer);
	void disable_timer(unsigned timer);

	void callback();

	uint8_t m_timers;
	uint8_t m_index;

	uint64_t m_period;
	volatile uint64_t *m_base;
};


