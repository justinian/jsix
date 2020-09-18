#include "objects/endpoint.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "page_manager.h"
#include "scheduler.h"
#include "vm_space.h"

endpoint::endpoint() :
	kobject(kobject::type::endpoint)
{}

endpoint::~endpoint()
{
	if (!check_signal(j6_signal_endpoint_closed))
		close();
}

void
endpoint::close()
{
	assert_signal(j6_signal_endpoint_closed);
	for (auto &data : m_blocked)
		data.th->wake_on_result(this, j6_status_closed);
}

j6_status_t
endpoint::send(size_t len, void *data)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread_data sender = { thread::from_tcb(tcb), data };
	sender.len = len;

	if (!check_signal(j6_signal_endpoint_can_send)) {
		assert_signal(j6_signal_endpoint_can_recv);
		sender.th->wait_on_object(this);
		m_blocked.append(sender);
		s.schedule();

		// we woke up having already finished the send
		// because it happened in the receiver
		return sender.th->get_wait_result();
	}

	thread_data receiver = m_blocked.pop_front();
	if (m_blocked.count() == 0)
		deassert_signal(j6_signal_endpoint_can_send);

	j6_status_t status = do_message_copy(sender, receiver);

	receiver.th->wake_on_result(this, status);
	return status;
}

j6_status_t
endpoint::receive(size_t *len, void *data)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread_data receiver = { thread::from_tcb(tcb), data };
	receiver.len_p = len;

	if (!check_signal(j6_signal_endpoint_can_recv)) {
		assert_signal(j6_signal_endpoint_can_send);
		receiver.th->wait_on_object(this);
		m_blocked.append(receiver);
		s.schedule();

		// we woke up having already finished the recv
		// because it happened in the sender
		return receiver.th->get_wait_result();
	}

	thread_data sender = m_blocked.pop_front();
	if (m_blocked.count() == 0)
		deassert_signal(j6_signal_endpoint_can_recv);

	// TODO: don't pop sender on some errors
	j6_status_t status = do_message_copy(sender, receiver);
	sender.th->wake_on_result(this, status);
	return status;
}

j6_status_t
endpoint::do_message_copy(const endpoint::thread_data &sender, endpoint::thread_data &receiver)
{
	if (sender.len > *receiver.len_p)
		return j6_err_insufficient;

	page_manager *pm = page_manager::get();
	vm_space &source = sender.th->parent().space();
	vm_space &dest = receiver.th->parent().space();
	vm_space::copy(source, dest, sender.data, receiver.data, sender.len);
	*receiver.len_p = sender.len;

	// TODO: this will not work if non-contiguous pages are mapped!!

	return j6_status_ok;
}
