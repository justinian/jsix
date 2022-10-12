#pragma once
/// \file mailbox.h
/// Definition of mailbox kobject types

#include <j6/cap_flags.h>
#include <util/counted.h>
#include <util/map.h>
#include <util/spinlock.h>

#include "objects/kobject.h"
#include "slab_allocated.h"
#include "wait_queue.h"

namespace obj {

class thread;

/// mailboxs are objects that enable synchronous message-passing IPC
class mailbox :
    public kobject
{
public:
    /// Capabilities on a newly constructed mailbox handle
    constexpr static j6_cap_t creation_caps = j6_cap_mailbox_all;

    static constexpr kobject::type type = kobject::type::mailbox;

    /// Max message handle count
    constexpr static size_t max_handle_count = 5;

    struct message;

    mailbox();
    virtual ~mailbox();

    /// Close the mailbox, waking all waiting processes with an error
    void close();

    /// Check if the mailbox has been closed
    inline bool closed() const { return m_closed; }

    /// Send a message to a thread waiting to receive on this mailbox, and block the
    /// current thread awaiting a response. The response will be placed in the message
    /// object provided.
    /// \arg msg      [inout] The mailbox::message to send, will contain the response afterward
    /// \returns      true if a reply was recieved
    bool call(message *msg);

    /// Receive the next available message, optionally blocking if no messages are available.
    /// \arg msg          [out] a pointer to the received message. The caller is responsible for
    ///                   deleting the message structure when finished.
    /// \arg block        True if this call should block when no messages are available.
    /// \returns          True if a message was received successfully.
    bool receive(message *&msg, bool block);

    class replyer;

    /// Find a given pending message to be responded to. Returns a replyer object, which will
    /// wake the calling thread upon destruction.
    /// \arg reply_tag  The reply tag in the original message
    /// \returns        A replyer object contining the message
    replyer reply(uint16_t reply_tag);

private:
    inline uint16_t next_reply_tag() {
        return (__atomic_add_fetch(&m_next_reply_tag, 1, __ATOMIC_SEQ_CST) << 1) | 1;
    }

    bool m_closed;
    uint16_t m_next_reply_tag;

    util::spinlock m_message_lock;
    util::deque<message*> m_messages;

    struct pending { thread *sender; message *msg; };
    util::map<uint16_t, pending> m_pending;
    wait_queue m_queue;
};


struct mailbox::message :
    public slab_allocated<message, 1>
{
    uint64_t tag;
    uint64_t subtag;

    uint16_t reply_tag;
    uint8_t handle_count;

    j6_handle_t handles[mailbox::max_handle_count];
};

class mailbox::replyer
{
public:
    replyer() : msg {nullptr}, caller {nullptr}, status {0} {}
    replyer(mailbox::message *m, thread *c, uint64_t s) : msg {m}, caller {c}, status {s} {}
    replyer(replyer &&o) : msg {o.msg}, caller {o.caller}, status {o.status} {
        o.msg = nullptr; o.caller = nullptr; o.status = 0;
    }

    replyer & operator=(replyer &&o) {
        msg = o.msg; caller = o.caller; status = o.status;
        o.msg = nullptr; o.caller = nullptr;
        return *this;
    }

    /// The replyer's dtor will wake the calling thread
    ~replyer();

    /// Check if the reply is valid
    inline bool valid() const { return msg && caller; }

    /// Set an error to give to the caller
    inline void error(uint64_t e) { status = e; }

    mailbox::message *msg;

private:
    thread *caller;
    uint64_t status;
};

} // namespace obj
