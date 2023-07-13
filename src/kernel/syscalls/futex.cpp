#include <j6/errors.h>
#include <util/basic_types.h>
#include <util/node_map.h>
#include <util/spinlock.h>

#include "clock.h"
#include "logger.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "vm_space.h"

using namespace obj;

namespace syscalls {

struct futex
{
    uintptr_t address;
    wait_queue queue;

    futex() = default;
    futex(futex &&other) :
        address {other.address},
        queue {util::move(other.queue)} {}

    futex & operator=(futex &&other) {
        address = other.address;
        queue = util::move(other.queue);
        return *this;
    }
};

inline uint64_t & get_map_key(futex &f) { return f.address; }

util::node_map<uintptr_t, futex> g_futexes;
util::spinlock g_futexes_lock;

j6_status_t
futex_wait(const uint32_t *value, uint32_t expected, uint64_t timeout)
{
    thread& t = thread::current();
    process &p = t.parent();

    if (*value != expected) {
        log::spam(logs::syscall, "<%02x:%02x> futex %lx: %x != %x", p.obj_id(), t.obj_id(), value, *value, expected);
        return j6_status_futex_changed;
    }

    uintptr_t address = reinterpret_cast<uintptr_t>(value);
    vm_space &space = process::current().space();
    uintptr_t phys = space.find_physical(address);

    util::scoped_lock lock {g_futexes_lock};

    futex &f = g_futexes[phys];

    if (timeout) {
        timeout += clock::get().value();
        t.set_wake_timeout(timeout);
    }

    log::spam(logs::syscall, "<%02x:%02x> blocking on futex %lx", p.obj_id(), t.obj_id(), value);

    lock.release();
    f.queue.wait();

    log::spam(logs::syscall, "<%02x:%02x> woke on futex %lx", p.obj_id(), t.obj_id(), value);
    return j6_status_ok;
}

j6_status_t
futex_wake(const uint32_t *value, size_t count)
{
    uintptr_t address = reinterpret_cast<uintptr_t>(value);
    vm_space &space = process::current().space();
    uintptr_t phys = space.find_physical(address);

    util::scoped_lock lock {g_futexes_lock};

    futex *f = g_futexes.find(phys);
    if (f) {
        if (count) {
            for (unsigned i = 0; i < count; ++i) {
                obj::thread *t = f->queue.pop_next();
                if (!t) break;
                t->wake();
            }
        } else {
            f->queue.clear();
        }

        if (f->queue.empty())
            g_futexes.erase(phys);
    }

    return j6_status_ok;
}

} // namespace syscalls
