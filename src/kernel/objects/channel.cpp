#include <string.h>

#include "assert.h"
#include "memory.h"
#include "objects/channel.h"
#include "objects/vm_area.h"

extern obj::vm_area_guarded g_kernel_buffers;

namespace obj {

constexpr size_t buffer_bytes = mem::kernel_buffer_pages * mem::frame_size;

channel::channel() :
    m_len(0),
    m_data(g_kernel_buffers.get_section()),
    m_buffer(reinterpret_cast<uint8_t*>(m_data), buffer_bytes),
    kobject(kobject::type::channel, j6_signal_channel_can_send)
{
}

channel::~channel()
{
    if (!closed()) close();
}

j6_status_t
channel::enqueue(size_t *len, const void *data)
{
    // TODO: Make this thread safe!
    if (closed())
        return j6_status_closed;

    if (!len || !*len)
        return j6_err_invalid_arg;

    if (m_buffer.free_space() == 0)
        return j6_err_not_ready;

    void *buffer = nullptr;
    size_t avail = m_buffer.reserve(*len, &buffer);
    *len = *len > avail ? avail : *len;

    memcpy(buffer, data, *len);
    m_buffer.commit(*len);

    assert_signal(j6_signal_channel_can_recv);
    if (m_buffer.free_space() == 0)
        deassert_signal(j6_signal_channel_can_send);

    return j6_status_ok;
}

j6_status_t
channel::dequeue(size_t *len, void *data)
{
    // TODO: Make this thread safe!
    if (closed())
        return j6_status_closed;

    if (!len || !*len)
        return j6_err_invalid_arg;

    if (m_buffer.size() == 0)
        return j6_err_not_ready;

    void *buffer = nullptr;
    size_t avail = m_buffer.get_block(&buffer);
    *len = *len > avail ? avail : *len;

    memcpy(data, buffer, *len);
    m_buffer.consume(*len);

    assert_signal(j6_signal_channel_can_send);
    if (m_buffer.size() == 0)
        deassert_signal(j6_signal_channel_can_recv);

    return j6_status_ok;
}

void
channel::close()
{
    kobject::close();
    g_kernel_buffers.return_section(m_data);
}

void
channel::on_no_handles()
{
    kobject::on_no_handles();
    delete this;
}

} // namespace obj
