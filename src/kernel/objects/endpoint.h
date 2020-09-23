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
	void close();

	/// Check if the endpoint has space for a message to be sent
	inline bool can_send() const { return check_signal(j6_signal_endpoint_can_send); }

	/// Check if the endpoint has a message wiating already
	inline bool can_receive() const { return check_signal(j6_signal_endpoint_can_recv); }

	/// Send a message to a thread waiting to receive on this endpoint. If no threads
	/// are currently trying to receive, block the current thread.
	/// \arg len   The size in bytes of the message
	/// \arg data  The message data
	/// \returns   j6_status_ok on success
	j6_status_t send(size_t len, void *data);

	/// Receive a message from a thread waiting to send on this endpoint. If no threads
	/// are currently trying to send, block the current thread.
	/// \arg len   [in] The size in bytes of the buffer [out] Number of bytes in the message
	/// \arg data  Buffer for copying message data into
	/// \returns   j6_status_ok on success
	j6_status_t receive(size_t *len, void *data);

private:
	struct thread_data
	{
		thread *th;
		void *data;
		union {
			size_t *len_p;
			size_t len;
		};
	};

	j6_status_t do_message_copy(const thread_data &sender, thread_data &receiver);

	kutil::vector<thread_data> m_blocked;
};
