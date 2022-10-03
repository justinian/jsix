#pragma once
/// \file node_map.h
/// Definition of a hash table collection for use in kernel space, where the values
/// are the hash nodes themselves - the hash key is part of the value.
///
/// Thanks to the following people for inspiration of this implementation:
///
///  Sebastian Sylvan
///  https://www.sebastiansylvan.com/post/robin-hood-hashing-should-be-your-default-hash-table-implementation/
///
///  Emmanuel Goossaert
///  http://codecapsule.com/2013/11/11/robin-hood-hashing/
///  http://codecapsule.com/2013/11/17/robin-hood-hashing-backward-shift-deletion/

#include <stdint.h>
#include <string.h>
#include <utility>
#include <util/hash.h>

namespace util {

using growth_func = void*(*)(void*, size_t, size_t);

inline void * default_realloc(void *p, size_t oldsize, size_t newsize) {
    char *newp = new char[newsize];
    memcpy(newp, p, oldsize);
    delete [] reinterpret_cast<char*>(p);
    return newp;
}

inline void * null_realloc(void *p, size_t oldsize, size_t newsize) { return p; }

/// Hash map where the values are the hash nodes themselves. (ie, the
/// hash key is a part of the value.) Growth is done with realloc, so
/// the map tries to grow in-place if it can.
template<typename K, typename V, K invalid_id = 0,
    growth_func realloc = default_realloc>
class node_map
{
public:
    using key_type = K;
    using node_type = V;

    static constexpr size_t max_load = 90;
    static constexpr size_t min_capacity = 8;

    inline size_t count() const { return m_count; }
    inline size_t capacity() const { return m_capacity; }
    inline size_t threshold() const { return (m_capacity * max_load) / 100; }

    /// Default constructor. Creates an empty map with the given capacity.
    node_map(size_t capacity = 0) :
        m_count {0},
        m_capacity {0},
        m_nodes {nullptr}
    {
        if (capacity) {
            m_capacity = 1 << log2(capacity);
            m_nodes = reinterpret_cast<node_type*>(
                    realloc(nullptr, 0, m_capacity * sizeof(node_type)));
            for (size_t i = 0; i < m_capacity; ++i)
                get_map_key(m_nodes[i]) = invalid_id;
        }
    }

    /// Existing buffer constructor. Uses the given buffer as initial
    /// capacity.
    node_map(node_type *buffer, size_t capacity) :
        m_count {0},
        m_capacity {capacity},
        m_nodes {buffer}
    {
        for (size_t i = 0; i < m_capacity; ++i)
            get_map_key(m_nodes[i]) = invalid_id;
    }

    virtual ~node_map() {
        for (size_t i = 0; i < m_capacity; ++i)
            m_nodes[i].~node_type();
        delete [] reinterpret_cast<uint8_t*>(m_nodes);
    }

    node_type & operator[](const key_type &key) {
        size_t slot;
        if (lookup(key, slot))
            return m_nodes[slot];

        node_type new_node;
        get_map_key(new_node) = key;
        return insert(std::move(new_node));
    }

    node_type * find(const key_type &key) {
        size_t slot;
        if (!lookup(key, slot))
            return nullptr;
        return &m_nodes[slot];
    }

    const node_type * find(const key_type &key) const {
        size_t slot;
        if (!lookup(key, slot))
            return false;
        return &m_nodes[slot];
    }

    node_type & insert(node_type&& node) {
        if (++m_count > threshold()) grow();

        key_type &key = get_map_key(node);
        size_t slot = mod(hash(key));
        size_t dist = 0;

        while (true) {
            node_type &node_at_slot = m_nodes[slot];
            key_type &key_at_slot = get_map_key(node_at_slot);

            if (open(key_at_slot)) {
                node_at_slot = node;
                return node_at_slot;
            }

            size_t psl_at_slot = psl(key_at_slot, slot);
            if (dist > psl_at_slot) {
                std::swap(node, node_at_slot);
                dist = psl_at_slot;
            }

            slot = mod(slot + 1);
            ++dist;
        }
    }

    bool erase(const key_type &key) {
        size_t slot;
        if (!lookup(key, slot))
            return false;

        node_type &node = m_nodes[slot];
        node.~node_type();
        get_map_key(node) = invalid_id;
        --m_count;

        while (fixup(slot++));
        return true;
    }

protected:
    inline size_t mod(size_t slot) const { return slot & (m_capacity - 1); }
    inline bool open(const key_type &key) const { return key == invalid_id; }

    inline size_t psl(const key_type &key, size_t slot) const {
        return mod(slot + m_capacity - mod(hash(key)));
    }

    bool fixup(size_t slot) {
        size_t next_slot = mod(slot+1);
        node_type &next = m_nodes[next_slot];
        key_type &next_key = get_map_key(next);

        if (open(next_key) || psl(next_key, next_slot) == 0)
            return false;

        m_nodes[slot] = std::move(next);
        next.~node_type();
        next_key = invalid_id;
        return true;
    }

    void grow() {
        node_type *old_nodes = m_nodes;
        size_t old_capacity = m_capacity;
        size_t new_capacity = m_capacity * 2;

        if (new_capacity < min_capacity)
            new_capacity = min_capacity;

        m_nodes = reinterpret_cast<node_type*>(
                realloc(m_nodes, old_capacity * sizeof(node_type),
                    new_capacity * sizeof(node_type)));

        for (size_t i = old_capacity; i < new_capacity; ++i)
            get_map_key(m_nodes[i]) = invalid_id;

        m_capacity = new_capacity;

        for (size_t slot = 0; slot < old_capacity; ++slot) {
            node_type &node = m_nodes[slot];
            key_type &key = get_map_key(node);
            size_t target = mod(hash(key));

            if (open(key) || target < old_capacity)
                continue;

            --m_count;
            insert(std::move(node));
            node.~node_type();
            key = invalid_id;
            
            size_t fixer = slot;
            while (fixup(fixer++) && fixer < old_capacity);
        }
    }

    const bool lookup(const key_type &key, size_t &slot) const {
        if (!m_count)
            return false;

        size_t dist = 0;
        slot = mod(hash(key));

        while (true) {
            key_type &key_at_slot = get_map_key(m_nodes[slot]);

            if (key_at_slot == key)
                return true;

            if (open(key_at_slot) || dist > psl(key_at_slot, slot))
                return false;

            slot = mod(slot + 1);
            ++dist;
        }
    }

private:
    size_t m_count;
    size_t m_capacity;
    node_type *m_nodes;
};

template <typename K, typename V, K invalid_id = 0>
using inplace_map = node_map<K, V, invalid_id, null_realloc>;

} // namespace util
