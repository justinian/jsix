#include "kernel_memory.h"

#include "assert.h"
#include "device_manager.h"
#include "hpet.h"
#include "io.h"
#include "log.h"

namespace {
    inline uint64_t volatile *capabilities(uint64_t volatile *base) { return base; }
    inline uint64_t volatile *configuration(uint64_t volatile *base) { return base+2; }
    inline uint64_t volatile *interrupt_status(uint64_t volatile *base) { return base+4; }
    inline uint64_t volatile *counter_value(uint64_t volatile *base) { return base+30; }

    inline uint64_t volatile *timer_base(uint64_t volatile *base, unsigned i) { return base + 0x20 + (4*i); }
    inline uint64_t volatile *timer_config(uint64_t volatile *base, unsigned i) { return timer_base(base, i); }
    inline uint64_t volatile *timer_comparator(uint64_t volatile *base, unsigned i) { return timer_base(base, i) + 1; }
}


void
hpet_irq_callback(void *hpet_ptr)
{
    if (hpet_ptr)
        reinterpret_cast<hpet*>(hpet_ptr)->callback();
}


hpet::hpet(uint8_t index, uint64_t *base) :
    m_index(index),
    m_base(base)
{
    *configuration(m_base) = 0;
    *counter_value(m_base) = 0;

    uint64_t caps = *capabilities(base);
    uint64_t config = *configuration(base);
    m_timers = ((caps >> 8) & 0x1f) + 1;
    m_period = (caps >> 32);

    setup_timer_interrupts(0, 2, 1000, true);
    // bool installed = device_manager::get()
    //  .install_irq(2, "HPET Timer", hpet_irq_callback, this);
    // kassert(installed, "Installing HPET IRQ handler");

    log::debug(logs::timer, "HPET %d capabilities:", index);
    log::debug(logs::timer, "       revision: %d", caps & 0xff);
    log::debug(logs::timer, "         timers: %d", m_timers);
    log::debug(logs::timer, "           bits: %d", ((caps >> 13) & 1) ? 64 : 32);
    log::debug(logs::timer, "    LRR capable: %d", ((caps >> 15) & 1));
    log::debug(logs::timer, "         period: %dns", m_period / 1000000);
    log::debug(logs::timer, " global enabled: %d", config & 1);
    log::debug(logs::timer, "     LRR enable: %d", (config >> 1) & 1);

    for (unsigned i = 0; i < m_timers; ++i) {
        disable_timer(i);
        uint64_t config = *timer_config(m_base, i);

        log::debug(logs::timer, "HPET %d timer %d:", index, i);
        log::debug(logs::timer, "       int type: %d", (config >> 1) & 1);
        log::debug(logs::timer, "     int enable: %d", (config >> 2) & 1);
        log::debug(logs::timer, "     timer type: %d", (config >> 3) & 1);
        log::debug(logs::timer, "   periodic cap: %d", (config >> 4) & 1);
        log::debug(logs::timer, "           bits: %d", ((config >> 5) & 1) ? 64 : 32);
        log::debug(logs::timer, "        32 mode: %d", (config >> 8) & 1);
        log::debug(logs::timer, "      int route: %d", (config >> 9) & 0x1f);
        log::debug(logs::timer, "     FSB enable: %d", (config >> 14) & 1);
        log::debug(logs::timer, "    FSB capable: %d", (config >> 15) & 1);
        log::debug(logs::timer, "     rotung cap: %08x", (config >> 32));
    }

}

void
hpet::setup_timer_interrupts(unsigned timer, uint8_t irq, uint64_t interval, bool periodic)
{
    constexpr uint64_t femto_per_us = 1000000000ull;
    *timer_comparator(m_base, timer) =
        *counter_value(m_base) +
        (interval * femto_per_us) / m_period;

    *timer_config(m_base, timer) = (irq << 9) | ((periodic ? 1 : 0) << 3);
}

void
hpet::enable_timer(unsigned timer)
{
    *timer_config(m_base, timer) = *timer_config(m_base, timer) | (1 << 2);
}

void
hpet::disable_timer(unsigned timer)
{
    *timer_config(m_base, timer) = *timer_config(m_base, timer) & ~(1ull << 2);
}

void
hpet::callback()
{
    log::debug(logs::timer, "HPET %d got irq", m_index);
}

void
hpet::enable()
{
    log::debug(logs::timer, "HPET %d enabling", m_index);
    *configuration(m_base) = (*configuration(m_base) & 0x3) | 1;
}

uint64_t
hpet::value() const
{
    return *counter_value(m_base);
}

uint64_t
hpet_clock_source(void *data)
{
    return reinterpret_cast<hpet*>(data)->value();
}
