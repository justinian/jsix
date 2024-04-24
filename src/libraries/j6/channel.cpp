// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <arch/memory.h>
#include <j6/channel.hh>
#include <j6/condition.hh>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/memutils.h>
#include <j6/mutex.hh>
#include <j6/syscalls.h>
#include <j6/syslog.hh>
#include <util/spinlock.h>
#include <util/bip_buffer.h>
#include <util/new.h>

namespace j6 {

struct channel_memory_area
{
    j6::mutex mutex;
    j6::condition waiting;
    util::bip_buffer buf;
};


static bool
check_channel_size(size_t s)
{
    if (s < arch::frame_size || (s & (s - 1)) != 0) {
        syslog(j6::logs::ipc, j6::log_level::error, "Bad channel size: %lx", s);
        return false;
    }
    return true;
}


static uintptr_t
create_channel_vma(j6_handle_t &vma, size_t size)
{
    uintptr_t addr = 0;
    j6_status_t result = j6_vma_create_map(&vma, size, &addr, j6_vm_flag_write);
    if (result != j6_status_ok) {
        syslog(j6::logs::ipc, j6::log_level::error, "Failed to create channel VMA. Error: %lx", result);
        return 0;
    }
    syslog(j6::logs::ipc, j6::log_level::verbose, "Created channel VMA at 0x%lx size 0x%lx", addr, size);
    return addr;
}


static void
init_channel_memory_area(channel_memory_area *area, size_t size)
{
    new (&area->mutex) j6::mutex;
    new (&area->waiting) j6::condition;
    new (&area->buf) util::bip_buffer {size - sizeof(*area)};
}


channel *
channel::create(size_t tx_size, size_t rx_size)
{
    if (!rx_size)
        rx_size = tx_size;

    if (!check_channel_size(tx_size) || !check_channel_size(rx_size))
        return nullptr;

    j6_status_t result;
    j6_handle_t tx_vma = j6_handle_invalid;
    j6_handle_t rx_vma = j6_handle_invalid;

    uintptr_t tx_addr = create_channel_vma(tx_vma, tx_size);
    if (!tx_addr)
        return nullptr;

    uintptr_t rx_addr = create_channel_vma(rx_vma, rx_size);
    if (!rx_addr) {
        j6_vma_unmap(tx_vma, 0);
        j6_handle_close(tx_vma);
        return nullptr;
    }

    channel_memory_area *tx = reinterpret_cast<channel_memory_area*>(tx_addr);
    channel_memory_area *rx = reinterpret_cast<channel_memory_area*>(rx_addr);
    init_channel_memory_area(tx, tx_size);
    init_channel_memory_area(rx, rx_size);

    j6::syslog(logs::ipc, log_level::info, "Created new channel with handles {%x, %x}", tx_vma, rx_vma);

    return new channel {{tx_vma, rx_vma}, *tx, *rx};
}

channel *
channel::open(const channel_def &def)
{
    j6_status_t result;

    uintptr_t tx_addr = 0;
    result = j6_vma_map(def.tx, 0, &tx_addr, 0);
    if (result != j6_status_ok) {
        syslog(j6::logs::ipc, j6::log_level::error, "Failed to map channel VMA. Error: %lx", result);
        return nullptr;
    }

    uintptr_t rx_addr = 0;
    result = j6_vma_map(def.rx, 0, &rx_addr, 0);
    if (result != j6_status_ok) {
        j6_vma_unmap(def.tx, 0);
        j6_handle_close(def.tx);
        syslog(j6::logs::ipc, j6::log_level::error, "Failed to map channel VMA. Error: %lx", result);
        return nullptr;
    }

    channel_memory_area *tx = reinterpret_cast<channel_memory_area*>(tx_addr);
    channel_memory_area *rx = reinterpret_cast<channel_memory_area*>(rx_addr);

    j6::syslog(logs::ipc, log_level::info, "Opening existing channel with handles {%x:0x%lx, %x:0x%lx}", def.tx, tx, def.rx, rx);
    return new channel { def, *tx, *rx };
}

channel::channel(
        const channel_def &def,
        channel_memory_area &tx,
        channel_memory_area &rx) :
    m_def {def},
    m_tx {tx},
    m_rx {rx}
{
}


size_t
channel::reserve(size_t size, uint8_t **area, bool block)
{
    if (size > m_tx.buf.buffer_size())
        return j6_err_insufficient;

    j6::scoped_lock lock {m_tx.mutex};
    while (m_tx.buf.write_available() < size) {
        if (!block) return 0;
        lock.release();
        m_tx.waiting.wait();
        lock.acquire();
    }

    return m_tx.buf.reserve(size, area);
}

void
channel::commit(size_t size)
{
    j6::syslog(logs::ipc, log_level::spam,
        "Sending %d bytes to channel on {%x}", size, m_def.tx);
    j6::scoped_lock lock {m_tx.mutex};
    m_tx.buf.commit(size);
    if (size)
        m_tx.waiting.wake();
}

size_t
channel::get_block(uint8_t const **area, bool block) const
{
    j6::scoped_lock lock {m_rx.mutex};
    while (!m_rx.buf.size()) {
        if (!block) return 0;

        lock.release();
        m_rx.waiting.wait();
        lock.acquire();
    }

    return m_rx.buf.get_block(area);
}

void
channel::consume(size_t size)
{
    j6::syslog(logs::ipc, log_level::spam,
        "Read %d bytes from channel on {%x}", size, m_def.rx);
    m_rx.buf.consume(size);
    if (size)
        m_rx.waiting.wake();
}

} // namespace j6

#endif // __j6kernel
