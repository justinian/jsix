#pragma once
/// \file channel.h
/// Definition of channel objects and related functions

#include <j6/signals.h>
#include <util/bip_buffer.h>

#include "objects/kobject.h"

/// Channels are bi-directional means of sending messages
class channel :
    public kobject
{
public:
    channel();
    virtual ~channel();

    static constexpr kobject::type type = kobject::type::channel;

    /// Check if the channel has space for a message to be sent
    inline bool can_send() const { return check_signal(j6_signal_channel_can_send); }

    /// Check if the channel has a message wiating already
    inline bool can_receive() const { return check_signal(j6_signal_channel_can_recv); }

    /// Put a message into the channel
    /// \arg len  [in] Bytes in data buffer [out] number of bytes written
    /// \arg data Pointer to the message data
    /// \returns  j6_status_ok on success
    j6_status_t enqueue(size_t *len, const void *data);

    /// Get a message from the channel, copied into a provided buffer
    /// \arg len  On input, the size of the provided buffer. On output,
    ///           the size of the message copied into the buffer.
    /// \arg data Pointer to the buffer
    /// \returns  j6_status_ok on success
    j6_status_t dequeue(size_t *len, void *data);

    /// Mark this channel as closed, all future calls to enqueue or
    /// dequeue messages will fail with j6_status_closed.
    virtual void close() override;

protected:
    virtual void on_no_handles() override;

private:
    size_t m_len;
    uintptr_t m_data;
    util::bip_buffer m_buffer;
};
