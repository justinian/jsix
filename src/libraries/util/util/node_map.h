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

#include <util/allocator.h>
#include <util/hash.h>
#include <util/util.h>

namespace util {

inline uint32_t & get_map_key(uint32_t &item) { return item; }
inline uint64_t & get_map_key(uint64_t &item) { return item; }

/// Hash map where the values are the hash nodes themselves. (ie, the
/// hash key is a part of the value.) Growth is done with realloc, so
/// the map tries to grow in-place if it can.
template<typename K, typename V, K invalid_id = 0,
    typename alloc = default_allocator>
class node_map
{
public:
    using key_type = K;
    using node_type = V;

    static constexpr size_t max_load = 90;
    static constexpr size_t min_capacity = 16;

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
                    alloc::allocate(m_capacity * sizeof(node_type)));
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

    class iterator
    {
    public:
        inline node_type & operator*() { return *node; }
        inline const node_type & operator*() const { return *node; }
        inline node_type * operator->() { return node; }
        inline const node_type * operator->() const { return node; }
        inline iterator & operator++() { incr(); return *this; }
        inline iterator operator++(int) { node_type *old = node; incr(); return iterator(old); }
        inline bool operator!=(const iterator &o) { return node != o.node; }
    private:
        friend class node_map;
        iterator(node_type *n) : node(n), end(n) {}
        iterator(node_type *n, node_type *end) : node(n), end(end) {}
        void incr() { while (node < end) if (get_map_key(*++node) != invalid_id) break; }
        node_type *node;
        node_type *end;
    };

    iterator begin() {
        if (!m_count) return iterator {0};
        iterator it {m_nodes - 1, m_nodes + m_capacity};
        return ++it;
    }

    const iterator begin() const {
        if (!m_count) return iterator {0};
        iterator it {m_nodes - 1, m_nodes + m_capacity};
        return ++it;
    }

    const iterator end() const {
        if (!m_count) return iterator {0};
        return iterator(m_nodes + m_capacity);
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
            return nullptr;
        return &m_nodes[slot];
    }

    node_type & insert(node_type&& node) {
        if (++m_count > threshold()) grow();

        key_type &key = get_map_key(node);
        size_t slot = mod(hash(key));
        size_t dist = 0;

        bool found = false;
        size_t inserted_at;

        while (true) {
            node_type &node_at_slot = m_nodes[slot];
            key_type &key_at_slot = get_map_key(node_at_slot);

            if (open(key_at_slot)) {
                node_at_slot = node;
                if (!found)
                    inserted_at = slot;
                return m_nodes[inserted_at];
            }

            size_t psl_at_slot = psl(key_at_slot, slot);
            if (dist > psl_at_slot) {
                if (!found) {
                    found = true;
                    inserted_at = slot;
                }
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

        fixup(slot);
        return true;
    }

protected:
    inline size_t mod(size_t slot) const { return slot & (m_capacity - 1); }
    inline bool open(const key_type &key) const { return key == invalid_id; }

    inline size_t psl(const key_type &key, size_t slot) const {
        return mod(slot + m_capacity - mod(hash(key)));
    }

    void fixup(size_t slot) {
        while (true) {
            size_t next_slot = mod(slot+1);
            node_type &next = m_nodes[next_slot];
            key_type &next_key = get_map_key(next);

            if (open(next_key) || psl(next_key, next_slot) == 0)
                return;

            m_nodes[slot] = std::move(next);
            next.~node_type();
            next_key = invalid_id;
            ++slot;
        }
    }

    void grow() {
        size_t old_capacity = m_capacity;
        size_t new_capacity = m_capacity * 2;

        if (new_capacity < min_capacity)
            new_capacity = min_capacity;

        void *grown = alloc::realloc(m_nodes,
                old_capacity * sizeof(node_type),
                new_capacity * sizeof(node_type));
        m_nodes = reinterpret_cast<node_type*>(grown);

        for (size_t i = old_capacity; i < new_capacity; ++i)
            get_map_key(m_nodes[i]) = invalid_id;

        m_capacity = new_capacity;

        for (size_t slot = 0; slot < old_capacity; ++slot) {
            node_type &node = m_nodes[slot];
            key_type &key = get_map_key(node);

            size_t target = mod(hash(key));

            if (open(key) || target <= slot)
                continue;

            --m_count;
            insert(std::move(node));
            node.~node_type();
            key = invalid_id;
        }

        for (size_t slot = old_capacity - 1; slot < old_capacity; --slot)
            if (open(get_map_key(m_nodes[slot])))
                fixup(slot);
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


/// A set container based on node_map
template <typename T, T invalid_id = 0, typename alloc = default_allocator>
class node_set
{
public:
    using item_type = T;
    using map_type = node_map<item_type, item_type, invalid_id, alloc>;

    /// Default constructor. Creates an empty set with the given capacity.
    node_set(size_t capacity = 0) : m_map(capacity) {}

    /// Existing buffer constructor. Uses the given buffer as initial
    /// capacity.
    node_set(item_type *buffer, size_t capacity) : m_map(buffer, capacity) {}

    /// Returns true if the given item is in the set.
    bool contains(const item_type &item) const { return m_map.find(item); }

    /// Add the given item to the set.
    /// \returns  True if the item was added, false if the item already existed
    bool add(item_type item) {
        if (contains(item))
            return false;
        m_map.insert(std::move(item));
        return true;
    }

    /// Remove the given item from the set.
    /// \returns  True if the item existed in the set
    bool remove(const item_type &item) { return m_map.erase(item); }

    const size_t count() const { return m_map.count(); }

    typename map_type::iterator begin() { return m_map.begin(); }
    const typename map_type::iterator begin() const { return m_map.begin(); }
    const typename map_type::iterator end() const { return m_map.end(); }

private:
    map_type m_map;
};

template <typename K, typename V, K invalid_id = 0>
using inplace_map = node_map<K, V, invalid_id, null_allocator>;

} // namespace util
