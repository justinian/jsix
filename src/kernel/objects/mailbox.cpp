#include <util/counted.h>

#include "objects/handle.h"
#include "objects/mailbox.h"
#include "objects/thread.h"

DEFINE_SLAB_ALLOCATOR(obj::mailbox::message);

namespace obj {

static_assert(mailbox::message::slab_size % sizeof(mailbox::message) == 0,
        "mailbox message size does not fit cleanly into N pages.");

constexpr uint64_t no_message = 1;

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
    util::scoped_lock lock {m_message_lock};

    // If this was previously closed, we're done
    if (closed()) return;
    m_closed = true;

    while (!m_messages.empty()) {
        message *msg = m_messages.pop_front();
        delete msg;
    }

    for (auto &p : m_pending) {
        delete p.val.msg;
        p.val.sender->wake(no_message);
    }
}

void
mailbox::send(message *msg)
{
    util::scoped_lock lock {m_message_lock};
    m_messages.push_back(msg);

    thread *t = m_queue.pop_next();
    if (t) t->wake();
}

bool
mailbox::call(message *msg)
{
    util::scoped_lock lock {m_message_lock};

    if (!++m_next_reply_tag) ++m_next_reply_tag;
    msg->reply_tag = m_next_reply_tag;

    thread &current = thread::current();
    m_pending.insert(m_next_reply_tag, {&current, msg});

    m_messages.push_back(msg);

    thread *t = m_queue.pop_next();
    if (t) t->wake();

    return (current.block() != no_message);
}

bool
mailbox::receive(mailbox::message *&msg, bool block)
{
    util::scoped_lock lock {m_message_lock};

    // This needs to be a loop because we're re-acquiring the lock
    // after waking up, and may have missed the message that woke us
    while (m_messages.empty()) {
        if (!block) {
            msg = nullptr;
            return false;
        }

        thread &cur = thread::current();
        m_queue.add_thread(&cur);

        lock.release();
        uint64_t result = cur.block();
        if (result == no_message)
            return false;
        lock.reacquire();
    }

    msg = m_messages.pop_front();
    return true;
}

mailbox::replyer
mailbox::reply(uint16_t reply_tag)
{
    util::scoped_lock lock {m_message_lock};

    pending *p = m_pending.find(reply_tag);
    if (!p) return {};

    thread *caller = p->sender;
    message *msg = p->msg;
    m_pending.erase(reply_tag);

    return {msg, caller};
}

mailbox::replyer::~replyer()
{
}

} // namespace obj
