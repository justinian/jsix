#include <string.h>

#include "assert.h"
#include "memory.h"
#include "objects/channel.h"
#include "objects/thread.h"
#include "objects/vm_area.h"

extern obj::vm_area_guarded g_kernel_buffers;

namespace obj {

constexpr size_t buffer_bytes = mem::kernel_buffer_pages * mem::frame_size;

channel::channel() :
    m_len       {0},
    m_data      {g_kernel_buffers.get_section()},
    m_closed    {false},
    m_buffer {reinterpret_cast<uint8_t*>(m_data), buffer_bytes},
    kobject {kobject::type::channel}
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
    len = m_buffer.reserve(len, &buffer);

    memcpy(buffer, data.pointer, len);
    m_buffer.commit(len);

    if (len) {
        thread *t = m_queue.pop_next();

        lock.release();
        if (t) t->wake();
    }

    return len;
}

size_t
channel::dequeue(util::buffer buffer, bool block)
{
    util::scoped_lock lock {m_close_lock};

    if (closed()) return 0;

    void *data = nullptr;
    size_t avail = m_buffer.get_block(&data);
    if (!avail && block) {
        thread &cur = thread::current();
        m_queue.add_thread(&cur);

        lock.release();
        cur.block();
        lock.reacquire();
        avail = m_buffer.get_block(&data);
    }

    size_t len = buffer.count > avail ? avail : buffer.count;

    memcpy(buffer.pointer, data, len);
    m_buffer.consume(len);

    return len;
}

void
channel::close()
{
    util::scoped_lock lock {m_close_lock};
    m_queue.clear();
    g_kernel_buffers.return_section(m_data);
    m_closed = true;
}

} // namespace obj
