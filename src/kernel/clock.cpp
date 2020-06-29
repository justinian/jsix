#include "clock.h"

clock * clock::s_instance = nullptr;

clock::clock(uint64_t rate, clock::source source_func, void *data) :
	m_rate(rate),
	m_data(data),
	m_source(source_func)
{
	// TODO: make this atomic
	if (s_instance == nullptr)
		s_instance = this;
	update();
}

void
clock::spinwait(uint64_t ns) const
{
	uint64_t when = m_source(m_data) + ns;
	while (value() < when);
}

