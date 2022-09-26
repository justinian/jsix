#pragma once
/// \file event.h
/// Definition of event kobject types

#include <stdint.h>
#include <j6/cap_flags.h>

#include "objects/kobject.h"
#include "wait_queue.h"

namespace obj {

class thread;

class event :
    public kobject
{
public:
    /// Capabilities on a newly constructed event handle
    constexpr static j6_cap_t creation_caps = j6_cap_event_all;
    static constexpr kobject::type type = kobject::type::event;

    event();
    ~event();

    /// Signal the given event(s).
    /// \arg s    The events to signal, as a bitset
    void signal(uint64_t s);

    /// Wait for events to be signalled. If events have already
    /// been signaled since the last call to wait, return them
    /// immediately.
    /// \returns  The events which have been signalled, as a bitset
    uint64_t wait();

private:
    /// Read and consume the current signaled events
    inline uint64_t read() {
        return __atomic_exchange_n(&m_signals, 0, __ATOMIC_SEQ_CST);
    }

    /// Wake an observer to handle an incoming event
    void wake_observer();

    uint64_t m_signals;
    wait_queue m_queue;
};

} // namespace obj
