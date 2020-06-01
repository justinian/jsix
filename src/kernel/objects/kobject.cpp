#include "j6/errors.h"
#include "j6/signals.h"
#include "j6/types.h"
#include "kutil/heap_allocator.h"
#include "objects/kobject.h"

extern kutil::heap_allocator g_kernel_heap;

// TODO: per-cpu this?
static j6_koid_t next_koid;

kobject::kobject(type t, j6_signal_t signals) :
	m_koid(koid_generate(t)),
	m_signals(signals),
	m_handle_count(0)
{}

kobject::~kobject()
{
	notify_signal_observers(0, j6_status_destroyed);
}

j6_koid_t
kobject::koid_generate(type t)
{
	return (static_cast<uint64_t>(t) << 48) | next_koid++;
}

kobject::type
kobject::koid_type(j6_koid_t koid)
{
	return static_cast<type>((koid >> 48) & 0xffffull);
}

void
kobject::assert_signal(j6_signal_t s)
{
	m_signals |= s;
	notify_signal_observers(s);
}

void
kobject::deassert_signal(j6_signal_t s)
{
	m_signals &= ~s;
	notify_signal_observers(s);
}

void
kobject::register_signal_observer(observer *object, j6_signal_t s)
{
	m_observers.emplace(object, s);
}

void
kobject::deregister_signal_observer(observer *object)
{
	for (size_t i = 0; i < m_observers.count(); ++i) {
		auto &reg = m_observers[i];
		if (reg.object != object) continue;

		reg = m_observers[m_observers.count() - 1];
		m_observers.remove();
		break;
	}
}

void
kobject::notify_signal_observers(j6_signal_t mask, j6_status_t result)
{
	for (auto &reg : m_observers) {
		if (mask == 0 || (reg.signals & mask) != 0) {
			if (!reg.object->on_signals_changed(this, m_signals, mask, result))
				reg.object = nullptr;
		}
	}

	// Compact the observer list
	long last = m_observers.count() - 1;
	while (m_observers[last].object == nullptr && last >= 0) last--;

	for (long i = 0; i < long(m_observers.count()) && i < last; ++i) {
		auto &reg = m_observers[i];
		if (reg.object != nullptr) continue;
		reg = m_observers[last--];

		while (m_observers[last].object == nullptr && last >= i) last--;
	}

	m_observers.set_size(last + 1);
}

void
kobject::on_no_handles()
{
	assert_signal(j6_signal_no_handles);
}
