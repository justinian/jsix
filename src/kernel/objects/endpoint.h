#pragma once
/// \file endpoint.h
/// Definition of endpoint kobject types

#include "j6/signals.h"
#include "objects/kobject.h"

/// Endpoints are objects that enable synchronous message-passing IPC
class endpoint :
    public kobject
{
public:
    endpoint();
    virtual ~endpoint();

    static constexpr kobject::type type = kobject::type::endpoint;

    /// Close the endpoint, waking all waiting processes with an error
    virtual void close() override;

    /// Check if the endpoint has space for a message to be sent
    inline bool can_send() const { return check_signal(j6_signal_endpoint_can_send); }

    /// Check if the endpoint has a message wiating already
    inline bool can_receive() const { return check_signal(j6_signal_endpoint_can_recv); }

    /// Send a message to a thread waiting to receive on this endpoint. If no threads
    /// are currently trying to receive, block the current thread.
    /// \arg tag   The application-specified message tag
    /// \arg data  The message data
    /// \arg len   The size in bytes of the message
    /// \returns   j6_status_ok on success
    j6_status_t send(j6_tag_t tag, const void *data, size_t data_len);

    /// Receive a message from a thread waiting to send on this endpoint. If no threads
    /// are currently trying to send, block the current thread.
    /// \arg tag   [in] The sender-specified message tag
    /// \arg len   [in] The size in bytes of the buffer [out] Number of bytes in the message
    /// \arg data  Buffer for copying message data into
    /// \returns   j6_status_ok on success
    j6_status_t receive(j6_tag_t *tag, void *data, size_t *data_len);

    /// Give the listener on the endpoint a message that a bound IRQ has been signalled
    /// \arg irq  The IRQ that caused this signal
    void signal_irq(unsigned irq);

private:
    struct thread_data
    {
        thread *th;
        union {
            const void *data;
            void *buffer;
        };
        union {
            j6_tag_t *tag_p;
            j6_tag_t tag;
        };
        union {
            size_t *len_p;
            size_t len;
        };
    };

    j6_status_t do_message_copy(const thread_data &sender, thread_data &receiver);

    kutil::vector<thread_data> m_blocked;
};
