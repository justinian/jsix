#if __has_include(<assert.h>)
#include <assert.h>
#else
#define assert(x) ((void)0)
#endif

#include <util/bip_buffer.h>

namespace util {

bip_buffer::bip_buffer() :
        m_start_a(0),
        m_start_b(0),
        m_size_a(0),
        m_size_b(0),
        m_size_r(0),
        m_buffer_size(0),
        m_buffer(nullptr)
    {}

bip_buffer::bip_buffer(uint8_t *buffer, size_t size) :
        m_start_a(0),
        m_start_b(0),
        m_size_a(0),
        m_size_b(0),
        m_size_r(0),
        m_buffer_size(size),
        m_buffer(buffer)
    {}

size_t bip_buffer::reserve(size_t size, void **area)
{
    scoped_lock lock {m_lock};

    if (m_size_r) {
        *area = nullptr;
        return 0;
    }

    size_t remaining = 0;
    if (m_size_b) {
        // If B exists, we're appending there. Get space between
        // the end of B and start of A.
        remaining = m_start_a - m_start_b - m_size_b;
        m_start_r = m_start_b + m_size_b;
    } else {
        // B doesn't exist, check the space both before and after A.
        // If the end of A has enough room for this write, put it there.
        remaining = m_buffer_size - m_start_a - m_size_a;
        m_start_r = m_start_a + m_size_a;

        // Otherwise use the bigger of the areas in front of and after A
        if (remaining < size && m_start_a > remaining) {
            remaining = m_start_a;
            m_start_r = 0;
        }
    }

    if (!remaining) {
        *area = nullptr;
        return 0;
    }

    m_size_r = (remaining < size) ? remaining : size;
    *area = &m_buffer[m_start_r];
    return m_size_r;
}

void bip_buffer::commit(size_t size)
{
    scoped_lock lock {m_lock};

    assert(size <= m_size_r && "Tried to commit more than reserved");
    if (m_size_r) {
        if (m_start_r == m_start_a + m_size_a) {
            // We were adding to A
            m_size_a += size;
        } else {
            // We were adding to B
            assert(m_start_r == m_start_b + m_size_b && "Bad m_start_r!");
            m_size_b += size;
        }
    }
    m_start_r = m_size_r = 0;
}

size_t bip_buffer::get_block(void **area) const
{
    scoped_lock lock {m_lock};

    *area = m_size_a ? &m_buffer[m_start_a] : nullptr;
    return m_size_a;
}

void bip_buffer::consume(size_t size)
{
    scoped_lock lock {m_lock};

    assert(size <= m_size_a && "Consumed more bytes than exist in A");
    if (size >= m_size_a) {
        if (m_size_r && m_start_r == m_start_a + m_size_a) {
            // A is still being appended to
            m_start_a = m_start_r;
            m_size_a = 0;
        } else {
            // A is done, B becomes A
            m_size_a = m_size_b;
            m_start_a = m_start_b;
            m_size_b = m_start_b = 0;
        }
    } else {
        m_size_a -= size;
        m_start_a += size;
    }
}

} // namespace util
