#pragma once
/// \file mailbox.h
/// Definition of mailbox kobject types

#include <j6/cap_flags.h>
#include <util/counted.h>
#include <util/node_map.h>
#include <util/spinlock.h>

#include "heap_allocator.h"
#include "memory.h"
#include "objects/kobject.h"
#include "objects/thread.h"
#include "slab_allocated.h"
#include "wait_queue.h"

namespace obj {

class thread;

/// mailboxs are objects that enable synchronous message-passing IPC
class mailbox :
    public kobject
{
public:
    using reply_tag_t = uint64_t;

    /// Capabilities on a newly constructed mailbox handle
    constexpr static j6_cap_t creation_caps = j6_cap_mailbox_all;

    static constexpr kobject::type type = kobject::type::mailbox;

    /// Max message handle count
    constexpr static size_t max_handle_count = 5;

    mailbox();
    virtual ~mailbox();

    /// Close the mailbox, waking all waiting processes with an error
    void close();

    /// Check if the mailbox has been closed
    inline bool closed() const { return __atomic_load_n(&m_closed, __ATOMIC_ACQUIRE); }

    /// Send a message to a thread waiting to receive on this mailbox, and block the
    /// current thread awaiting a response. The message contents should be in the calling
    /// thread's message_data.
    /// \returns      j6_status_ok if a reply was received
    j6_status_t call();

    /// Receive the next available message, optionally blocking if no messages are available.
    /// \arg data         [out] a thread::message_data structure to fill
    /// \arg reply_tag    [out] the reply_tag to use when replying to this message
    /// \arg block        True if this call should block when no messages are available.
    /// \returns          j6_status_ok if a message was received
    j6_status_t receive(thread::message_data &data, reply_tag_t &reply_tag, bool block);

    /// Find a given pending message to be responded to. Returns a replyer object, which will
    /// wake the calling thread upon destruction.
    /// \arg reply_tag  The reply tag in the original message
    /// \arg data       Message data to pass on to the caller
    /// \returns        j6_status_ok if the reply was successfully sent
    j6_status_t reply(reply_tag_t reply_tag, const thread::message_data &data);

private:
    wait_queue m_callers;
    wait_queue m_responders;

    struct reply_to { reply_tag_t reply_tag; thread *thread; };
    using reply_map =
        util::node_map<uint64_t, reply_to, 0, heap_allocated>;

    util::spinlock m_reply_lock;
    reply_map m_reply_map;

    bool m_closed;
    reply_tag_t m_next_reply_tag;

    friend reply_tag_t & get_map_key(reply_to &rt);
};

inline mailbox::reply_tag_t & get_map_key(mailbox::reply_to &rt) { return rt.reply_tag; }

} // namespace obj
