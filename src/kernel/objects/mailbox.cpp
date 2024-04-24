#include <util/basic_types.h>
#include <util/counted.h>
#include <j6/memutils.h>

#include "logger.h"
#include "objects/mailbox.h"
#include "objects/thread.h"

namespace obj {

mailbox::mailbox() :
    kobject(kobject::type::mailbox),
    m_closed {false},
    m_next_reply_tag {0}
{
}

mailbox::~mailbox()
{
    close();
}

void
mailbox::close()
{
    // If this was previously closed, we're done
    bool was_closed = __atomic_exchange_n(&m_closed, true, __ATOMIC_ACQ_REL);
    if (was_closed) return;

    log::spam(logs::ipc, "mbx[%2x] closing...", obj_id());

    m_callers.clear(j6_status_closed);
    m_responders.clear(j6_status_closed);

    util::scoped_lock lock {m_reply_lock};
    for (auto &waiting : m_reply_map)
        waiting.thread->wake(j6_status_closed);
}

j6_status_t
mailbox::call()
{
    if (closed())
        return j6_status_closed;

    thread &current = thread::current();
    m_callers.add_thread(&current);

    thread *responder = m_responders.pop_next();
    if (responder) {
        log::spam(logs::ipc, "thread[%2x]:: mbx[%2x] call() waking thread[%2x]...",
            current.obj_id(), obj_id(), responder->obj_id());
        responder->wake(j6_status_ok);
    } else {
        log::spam(logs::ipc, "thread[%2x]:: mbx[%2x] call() found no responder yet.",
            current.obj_id(), obj_id());
    }

    return current.block();
}

j6_status_t
mailbox::receive(ipc::message_ptr &data, reply_tag_t &reply_tag, bool block)
{
    if (closed())
        return j6_status_closed;

    thread &current = thread::current();
    thread *caller = nullptr;

    while (true) {
        caller = m_callers.pop_next();
        if (caller)
            break;

        if (!block)
            return j6_status_would_block;

        log::spam(logs::ipc, "thread[%2x]:: mbx[%2x] receive() blocking waiting for a caller",
            current.obj_id(), obj_id());

        m_responders.add_thread(&current);
        j6_status_t s = current.block();
        if (s != j6_status_ok)
            return s;
    }

    util::scoped_lock lock {m_reply_lock};
    reply_tag = ++m_next_reply_tag;
    m_reply_map.insert({ reply_tag, caller });
    lock.release();

    log::spam(logs::ipc, "thread[%2x]:: mbx[%2x] receive() found caller thread[%2x], rt = %x",
        current.obj_id(), obj_id(), caller->obj_id(), reply_tag);

    data = caller->get_message_data();
    return j6_status_ok;
}

j6_status_t
mailbox::reply(reply_tag_t reply_tag, ipc::message_ptr data)
{
    if (closed())
        return j6_status_closed;

    util::scoped_lock lock {m_reply_lock};
    reply_to *rt = m_reply_map.find(reply_tag);
    if (!rt)
        return j6_err_invalid_arg;

    thread *caller = rt->thread;
    m_reply_map.erase(reply_tag);
    lock.release();

    thread &current = thread::current();
    log::spam(logs::ipc, "thread[%2x]:: mbx[%2x] reply() to caller thread[%2x], rt = %x",
        current.obj_id(), obj_id(), caller->obj_id(), reply_tag);

    caller->set_message_data(util::move(data));
    caller->wake(j6_status_ok);
    return j6_status_ok;
}

} // namespace obj
