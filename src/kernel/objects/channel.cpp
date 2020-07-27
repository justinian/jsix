#include "kutil/assert.h"

#include "objects/channel.h"

channel::channel() :
	m_len(0),
	m_capacity(0),
	m_data(nullptr),
	kobject(kobject::type::channel, j6_signal_channel_can_send)
{
}

channel::~channel()
{
	kutil::kfree(m_data);
}

j6_status_t
channel::enqueue(size_t len, void *data)
{
	// TODO: Make this thread safe!
	if (closed())
		return j6_status_closed;

	if (!can_send())
		return j6_err_not_ready;

	if (m_capacity < len) {
		kutil::kfree(m_data);
		m_data = kutil::kalloc(len);
		m_capacity = len;
		kassert(m_data, "Failed to allocate memory to copy channel message");
	}

	m_len = len;
	kutil::memcpy(m_data, data, len);
	assert_signal(j6_signal_channel_can_recv);

	return j6_status_ok;
}

j6_status_t
channel::dequeue(size_t *len, void *data)
{
	// TODO: Make this thread safe!
	if (closed())
		return j6_status_closed;

	if (!can_receive())
		return j6_err_not_ready;

	if (!len)
		return j6_err_invalid_arg;

	if (*len < m_len)
		return j6_err_insufficient;

	kutil::memcpy(data, m_data, m_len);
	*len = m_len;
	assert_signal(j6_signal_channel_can_send);

	return j6_status_ok;
}

void
channel::on_no_handles()
{
	kobject::on_no_handles();
	delete this;
}
