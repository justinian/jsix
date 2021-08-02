#pragma once
/// \file clock.h
/// The kernel time keeping interface

#include <stdint.h>

class clock
{
public:
    /// A source is a function that returns the current
    /// value of some clock source.
    using source = uint64_t (*)(void*);

    /// Constructor.
    /// \arg rate   Number of source ticks per us
    /// \arg source Function for the clock source
    /// \arg data   Data to pass to the source function
    clock(uint64_t rate, source source_func, void *data);

    /// Get the current value of the clock.
    /// \returns Current value of the source, in us
    /// TODO: optimize divison by finding a multiply and
    /// shift value instead
    inline uint64_t value() const { return m_source(m_data) / m_rate; }

    /// Update the internal state via the source
    /// \returns Current value of the clock
    inline void update() { m_current = value(); }

    /// Wait in a tight loop
    /// \arg interval  Time to wait, in us
    void spinwait(uint64_t us) const;

    /// Get the master clock
    static clock & get() { return *s_instance; }

private:
    uint64_t m_current; ///< current us count
    uint64_t m_rate; ///< source ticks per us
    void *m_data;
    source m_source;

    static clock *s_instance;
};
