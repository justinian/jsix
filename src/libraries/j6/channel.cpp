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

namespace j6 {

static uintptr_t channel_addr = 0x6000'0000;
static util::spinlock addr_spinlock;

struct channel::header
{
    size_t size;
    size_t read_index;
    size_t write_index;

    mutex mutex;
    condition read_waiting;
    condition write_waiting;

    uint8_t data[0];

    inline const void* read_at() const { return &data[read_index & (size - 1)]; }
    inline void* write_at()            { return &data[write_index & (size - 1)]; }

    inline size_t read_avail() const   { return write_index - read_index; }
    inline size_t write_avail() const  { return size - read_avail(); }

    inline void consume(size_t n)      { read_index += n; }
    inline void commit(size_t n)       { write_index += n; }
};

channel *
channel::create(size_t size)
{
    j6_status_t result;
    j6_handle_t vma = j6_handle_invalid;

    if (size < arch::frame_size || (size & (size - 1)) != 0) {
        syslog("Bad channel size: %lx", size);
        return nullptr;
    }

    util::scoped_lock lock {addr_spinlock};
    uintptr_t addr = channel_addr;
    channel_addr += size * 2; // account for ring buffer virtual space doubling
    lock.release();

    result = j6_vma_create_map(&vma, size, &addr, j6_vm_flag_write|j6_vm_flag_ring);
    if (result != j6_status_ok) {
        syslog("Failed to create channel VMA. Error: %lx", result);
        return nullptr;
    }

    header *h = reinterpret_cast<header*>(addr);
    memset(h, 0, sizeof(*h));
    h->size = size;

    return new channel {vma, h};
}

channel *
channel::open(j6_handle_t vma)
{
    j6_status_t result;

    util::scoped_lock lock {addr_spinlock};
    uintptr_t addr = channel_addr;

    result = j6_vma_map(vma, 0, &addr, 0);
    if (result != j6_status_ok) {
        syslog("Failed to map channel VMA. Error: %lx", result);
        return nullptr;
    }

    header *h = reinterpret_cast<header*>(addr);
    channel_addr += h->size;
    lock.release();

    return new channel {vma, h};
}

channel::channel(j6_handle_t vma, header *h) :
    m_vma {vma},
    m_size {h->size},
    m_header {h}
{
}

j6_status_t
channel::send(const void *buffer, size_t len, bool block)
{
    if (len > m_header->size)
        return j6_err_insufficient;

    j6::scoped_lock lock {m_header->mutex};
    while (m_header->write_avail() < len) {
        if (!block)
            return j6_status_would_block;

        lock.release();
        m_header->write_waiting.wait();
        lock.acquire();
    }

    memcpy(m_header->write_at(), buffer, len);
    m_header->commit(len);
    m_header->read_waiting.wake();

    return j6_status_ok;
}

j6_status_t
channel::receive(void *buffer, size_t *size, bool block)
{
    j6::scoped_lock lock {m_header->mutex};
    while (!m_header->read_avail()) {
        if (!block)
            return j6_status_would_block;

        lock.release();
        m_header->read_waiting.wait();
        lock.acquire();
    }

    size_t avail = m_header->read_avail();
    size_t read = *size > avail ? avail : *size;

    memcpy(buffer, m_header->read_at(), read);
    m_header->consume(read);
    m_header->write_waiting.wake();

    *size = read;
    return j6_status_ok;
}

} // namespace j6

#endif // __j6kernel
