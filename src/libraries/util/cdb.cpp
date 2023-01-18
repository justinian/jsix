#include <util/cdb.h>

namespace util {

namespace {

struct pointer
{
    uint32_t position;
    uint32_t length;
};

struct slot
{
    uint32_t hash;
    uint32_t position;
};

struct record
{
    uint32_t keylen;
    uint32_t vallen;
    uint8_t data[];
};

static constexpr size_t min_length = 256 * sizeof(pointer);

inline uint32_t
djbhash(uint8_t const *key, uint32_t len)
{
    static constexpr uint32_t starting_hash = 5381;
    uint32_t h = starting_hash;
    for (unsigned i = 0; i < len; ++i)
        h = ((h << 5) + h) ^ key[i];
    return h;
}

inline bool
equal(uint8_t const *key1, size_t len1, uint8_t const *key2, uint32_t len2)
{
    if (len1 != len2)
        return false;

    for (unsigned i = 0; i < len1; ++i)
        if (key1[i] != key2[i])
            return false;

    return true;
}

// util cannot depend on libc
inline uint32_t strlen(const char *s) {
    uint32_t i = 0;
    while (s && *s++) ++i;
    return i;
}

} // anon namespace

cdb::cdb(buffer data) :
    m_data(data)
{
    if (data.count < min_length)
        m_data = {0, 0};
}

const buffer
cdb::retrieve(const char *key) const
{
    uint32_t len = strlen(key);
    return retrieve(reinterpret_cast<const uint8_t *>(key), len);
}

const buffer
cdb::retrieve(const uint8_t *key, uint32_t len) const
{
    if (!m_data.pointer || !m_data.count)
        return {0,0};

    uint32_t h = djbhash(key, len);
    uint32_t pindex = h & 0xff;

    pointer const *p = &at<pointer>(0)[pindex];

    if (!p->length)
        return {0, 0};

    uint32_t hindex = (h >> 8) % p->length;
    slot const *table = at<slot>(p->position);

    uint32_t i = hindex;
    slot const *s = &table[i];

    while (s->hash != 0) {
        if (s->hash == h) {
            record const *r = at<record>(s->position);
            if (equal(key, len, &r->data[0], r->keylen))
                return buffer::from_const( &r->data[r->keylen], r->vallen );
        }

        i = (i + 1) % p->length;
        if (i == hindex) break;
        s = &table[i];
    }

    return {0, 0};

}

} // namespace util
