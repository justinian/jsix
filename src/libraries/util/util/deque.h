#pragma once
/// \file linked_list.h
/// A generic templatized linked list.

#include <utility>
#include <assert.h>
#include <string.h>
#include <util/linked_list.h>

namespace util {

template <typename T, unsigned N = 16>
class deque
{
public:
    struct node { T items[N]; };
    using list_type = linked_list<node>;
    using node_type = typename list_type::item_type;

    class iterator
    {
    public:
        iterator(node_type *n, size_t i) : node(n), index(i) {
            if (node && index >= N) { node = node->next(); index = 0; }
        }
        iterator(const iterator &o) : node(o.node), index(o.index) {}
        inline T& operator*() { assert(node); return node->items[index]; }
        inline iterator & operator++() { incr(); return *this; }
        inline iterator & operator++(int) { iterator old {*this}; incr(); return old; }
        inline bool operator!=(const iterator &o) const { return !(*this == o); }
        inline bool operator==(const iterator &o) const {
            if (!node && !o.node) return true;
            return index == o.index && node == o.node;
        }
    private:
        void incr() { if (++index >= N) { index = 0; node = node->next(); }}
        node_type *node;
        size_t index;
    };

    deque() : m_first {0}, m_next {N} {}

    deque(deque &&other) :
        m_first {other.m_first},
        m_next {other.m_next},
        m_list {std::move(other.m_list)} {}

    ~deque() { clear(); }

    deque & operator=(deque &&other) {
        clear();
        m_first = other.m_first;
        m_next = other.m_next;
        m_list = std::move(other.m_list);
        return *this;
    }

    inline void push_front(const T& item) {
        if (!m_first) { // need a new block at the start
            node_type *n = new node_type;
            memset(n, 0, sizeof(node_type));
            m_list.push_front(n);
            m_first = N;
        }
        m_list.front()->items[--m_first] = item;
    }

    inline void push_back(const T& item) {
        if (m_next == N) { // need a new block at the end
            node_type *n = new node_type;
            memset(n, 0, sizeof(node_type));
            m_list.push_back(n);
            m_next = 0;
        }
        m_list.back()->items[m_next++] = item;
    }

    inline T pop_front() {
        assert(!empty() && "Calling pop_front() on an empty deque");
        T value = m_list.front()->items[m_first++];
        if (m_first == N) {
            delete m_list.pop_front();
            m_first = 0;
            if (m_list.empty())
                m_next = N;
        }
        return value;
    }

    inline T pop_back() {
        assert(!empty() && "Calling pop_back() on an empty deque");
        T value = m_list.back()->items[--m_next];
        if (m_next == 0) {
            delete m_list.pop_back();
            m_next = N;
            if (m_list.empty())
                m_first = 0;
        }
        return value;
    }

    inline bool empty() const {
        return m_list.empty() ||
            m_list.length() == 1 && m_first == m_next;
    }

    inline T& first() {
        assert(!empty() && "Calling first() on an empty deque");
        return m_list.front()->items[m_first];
    }

    inline const T& first() const {
        assert(!empty() && "Calling first() on an empty deque");
        return m_list.front()->items[m_first];
    }

    inline T& last() {
        assert(!empty() && "Calling last() on an empty deque");
        return m_list.back()->items[m_next-1];
    }

    inline const T& last() const {
        assert(!empty() && "Calling last() on an empty deque");
        return m_list.back()->items[m_next-1];
    }

    inline void clear() {
        while (!m_list.empty())
            delete m_list.pop_front();
        m_first = 0;
        m_next = N;
    }

    iterator begin() { return iterator {m_list.front(), m_first}; }
    iterator end() { return iterator {m_list.back(), m_next}; }

private:
    unsigned m_first; // Index of the first item in the first node
    unsigned m_next;  // Index of the first empty item in the last node
    list_type m_list;
};

} // namespace util
