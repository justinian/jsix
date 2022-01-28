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

size_t
channel::enqueue(const util::buffer &data)
{
    util::scoped_lock lock {m_close_lock};

    if (closed()) return 0;

    size_t len = data.count;
    void *buffer = nullptr;
    size_t avail = m_buffer.reserve(len, &buffer);

    len = len > avail ? avail : len;

    memcpy(buffer, data.pointer, len);
    m_buffer.commit(len);

    if (len)
        assert_signal(j6_signal_channel_can_recv);

    if (m_buffer.free_space() == 0)
        deassert_signal(j6_signal_channel_can_send);

    return len;
}

size_t
channel::dequeue(util::buffer buffer)
{
    util::scoped_lock lock {m_close_lock};

    if (closed()) return 0;

    void *data = nullptr;
    size_t avail = m_buffer.get_block(&data);
    size_t len = buffer.count > avail ? avail : buffer.count;

    memcpy(data, buffer.pointer, len);
    m_buffer.consume(len);

    if (len)
        assert_signal(j6_signal_channel_can_send);

    if (m_buffer.size() == 0)
        deassert_signal(j6_signal_channel_can_recv);

    return len;
}

void
channel::close()
{
    util::scoped_lock lock {m_close_lock};

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
