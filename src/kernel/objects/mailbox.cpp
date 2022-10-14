#include <util/counted.h>

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

    m_callers.clear(j6_status_closed);
    m_responders.clear(j6_status_closed);
}

j6_status_t
mailbox::call()
{
    if (closed())
        return j6_status_closed;

    thread &current = thread::current();
    m_callers.add_thread(&current);

    thread *responder = m_responders.pop_next();
    if (responder)
        responder->wake(j6_status_ok);

    return current.block();
}

j6_status_t
mailbox::receive(thread::message_data &data, reply_tag_t &reply_tag, bool block)
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

        m_responders.add_thread(&current);
        j6_status_t s = current.block();
        if (s != j6_status_ok)
            return s;
    }

    util::scoped_lock lock {m_reply_lock};
    reply_tag = ++m_next_reply_tag;
    m_reply_map.insert({ reply_tag, caller });
    lock.release();

    data = caller->get_message_data();
    return j6_status_ok;
}

j6_status_t
mailbox::reply(reply_tag_t reply_tag, const thread::message_data &data)
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

    caller->get_message_data() = data;
    caller->wake(j6_status_ok);
    return j6_status_ok;
}

} // namespace obj
