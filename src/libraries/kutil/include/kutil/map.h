#pragma once
/// \file map.h
/// Definition of a simple associative array collection for use in kernel space.
/// Thanks to the following people for inspiration of this implementation:
///
///  Sebastian Sylvan
///  https://www.sebastiansylvan.com/post/robin-hood-hashing-should-be-your-default-hash-table-implementation/
///
///  Emmanuel Goossaert
///  http://codecapsule.com/2013/11/11/robin-hood-hashing/
///  http://codecapsule.com/2013/11/17/robin-hood-hashing-backward-shift-deletion/

#include <stdint.h>
#include "kutil/hash.h"
#include "kutil/memory.h"
#include "kutil/vector.h"
#include "kutil/util.h"

namespace kutil {

/// Templated equality check to allow overriding
template <typename T>
inline bool equal(const T &a, const T &b) { return a == b; }

template <>
inline bool equal<const char *>(const char * const &a, const char * const &b) {
    if (!a || !b) return a == b;
    const char *a1 = a, *b1 = b;
    while (*a1 && *b1) if (*a1++ != *b1++) return false;
    return *a1 == *b1; // Make sure they're both zero
}

template <typename K, typename V>
struct hash_node
{
    uint64_t h {0};
    K key;
    V val;

    hash_node(hash_node &&o) : h(o.h), key(std::move(o.key)), val(std::move(o.val)) {}
    hash_node(uint64_t h, K &&k, V &&v) : h(h), key(std::move(k)), val(std::move(v)) {}
    ~hash_node() { h = 0; }

    inline uint64_t & hash() { return h; }
    inline uint64_t hash() const { return h; }
};

/// Base class for hash maps
template <typename K, typename V>
class base_map
{
protected:
    using node = hash_node<K, V>;

public:
    static constexpr size_t min_capacity = 8;
    static constexpr size_t max_load = 90;

    class iterator
    {
    public:
        inline node & operator*() { return *m_node; }
        inline node * operator->() { return m_node; }
        inline const node & operator*() const { return *m_node; }
        inline iterator & operator++() { incr(); return *this; }
        inline iterator operator++(int) { node *old = m_node; incr(); return iterator(old); }
        inline bool operator!=(const iterator &o) { return m_node != o.m_node; }
    private:
        friend class base_map;
        iterator(node *n) : m_node(n), m_end(n) {}
        iterator(node *n, node *end) : m_node(n), m_end(end) {}
        void incr() { while (m_node < m_end) { ++m_node; if (m_node->hash()) break; } }
        node *m_node;
        node *m_end;
    };

    /// Default constructor. Creates an empty map with the given capacity.
    base_map(size_t capacity = 0) :
        m_count(0),
        m_capacity(0),
        m_nodes(nullptr)
    {
        if (capacity)
            set_capacity(1 << log2(capacity));
    }

    virtual ~base_map() {
        for (size_t i = 0; i < m_capacity; ++i)
            m_nodes[i].~node();
        kfree(m_nodes);
    }

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

    void insert(K k, V v) {
        if (++m_count > threshold()) grow();
        insert_node(hash(k), std::move(k), std::move(v));
    }

    bool erase(const K &k)
    {
        node *n = lookup(k);
        if (!n) return false;

        n->~node();
        --m_count;

        size_t i = n - m_nodes;
        while (true) {
            size_t next = mod(i+1);
            node &m = m_nodes[next];
            if (!m.hash() || mod(m.hash()) == next) break;
            construct(i, m.hash(), std::move(m.key), std::move(m.val));
            m.~node();
            i = mod(++i);
        }

        return true;
    }

    inline size_t count() const { return m_count; }
    inline size_t capacity() const { return m_capacity; }
    inline size_t threshold() const { return (m_capacity * max_load) / 100; }

protected:
    inline size_t mod(uint64_t i) const { return i & (m_capacity - 1); }
    inline size_t offset(uint64_t h, size_t i) const {
        return mod(i + m_capacity - mod(h));
    }

    void set_capacity(size_t capacity) {
        kassert((capacity & (capacity - 1)) == 0,
                "Map capacity must be a power of two");

        m_capacity = capacity;
        const size_t size = m_capacity * sizeof(node);
        m_nodes = reinterpret_cast<node*>(kalloc(size));
        kutil::memset(m_nodes, 0, size);
    }

    void grow() {
        node *old = m_nodes;
        size_t count = m_capacity;

        size_t cap = m_capacity * 2;
        if (cap < min_capacity)
            cap = min_capacity;

        set_capacity(cap);

        for (size_t i = 0; i < count; ++i) {
            node &n = old[i];
            insert_node(n.hash(), std::move(n.key), std::move(n.val));
            n.~node();
        }

        kfree(old);
    }

    inline node * construct(size_t i, uint64_t h, K &&k, V &&v) {
        return new (&m_nodes[i]) node(h, std::move(k), std::move(v));
    }

    node * insert_node(uint64_t h, K &&k, V &&v) {
        size_t i = mod(h);
        size_t dist = 0;

        while (true) {
            if (!m_nodes[i].hash()) {
                return construct(i, h, std::move(k), std::move(v));
            }

            node &elem = m_nodes[i];
            size_t elem_dist = offset(elem.hash(), i);
            if (elem_dist < dist) {
                std::swap(h, elem.hash());
                std::swap(k, elem.key);
                std::swap(v, elem.val);
                dist = elem_dist;
            }

            i = mod(++i);
            ++dist;
        }
    }

    node * lookup(const K &k) {
        if (!m_count)
            return nullptr;

        uint64_t h = hash(k);
        size_t i = mod(h);
        size_t dist = 0;

        while (true) {
            node &n = m_nodes[i];
            if (!n.hash() || dist > offset(n.hash(), i))
                return nullptr;

            else if (n.hash() == h && equal(n.key, k))
                return &n;

            i = mod(++i);
            ++dist;
        }
    }

    const node * lookup(const K &k) const {
        if (!m_count)
            return nullptr;

        uint64_t h = hash(k);
        size_t i = mod(h);
        size_t dist = 0;

        while (true) {
            const node &n = m_nodes[i];
            if (!n.hash() || dist > offset(n.hash(), i))
                return nullptr;

            else if (n.hash() == h && equal(n.key, k))
                return &n;

            i = mod(++i);
            ++dist;
        }
    }

    size_t m_count;
    size_t m_capacity;
    node *m_nodes;
};

/// An open addressing hash map using robinhood hashing.
template <typename K, typename V>
class map :
    public base_map<K, V>
{
    using base = base_map<K, V>;
    using node = typename base::node;

public:
    map(size_t capacity = 0) :
        base(capacity) {}

    V * find(const K &k) {
        node *n = this->lookup(k);
        return n ? &n->val : nullptr;
    }

    const V * find(const K &k) const {
        const node *n = this->lookup(k);
        return n ? &n->val : nullptr;
    }
};

/// An open addressing hash map using robinhood hashing. Specialization
/// for storing pointers: don't return a pointer to a pointer.
template <typename K, typename V>
class map <K, V*> :
    public base_map<K, V*>
{
    using base = base_map<K, V*>;
    using node = typename base::node;

public:
    map(size_t capacity = 0) :
        base(capacity) {}

    V * find(const K &k) const {
        const node *n = this->lookup(k);
        return n ? n->val : nullptr;
    }
};

} // namespace kutil
