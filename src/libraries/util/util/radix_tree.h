#pragma once
/// \file radix_tree.h
/// Definition of a generic radix_tree structure

#include <stdint.h>
#include <j6/memutils.h>
#include <util/util.h>

namespace util {

template <typename T, size_t N = 64, uint8_t max_level = 5, unsigned extra_shift = 0>
class radix_tree
{
public:
    using node_type = radix_tree<T,N,max_level,extra_shift>;

    static_assert(sizeof(T) == sizeof(void*),
            "Only pointer-sized types are valid radix tree values.");

    /// Get the entry with the given key.
    /// \arg root   The root node of the tree
    /// \arg key    Key of the object to find
    /// \arg entry  [out] points to the entry if found, or null
    /// \returns    True if an entry was found
    static bool find(const node_type *root, uint64_t key, const T *entry) {
        node_type const *node = root;

        while (node) {
            uint8_t level = node->m_level;
            size_t index = 0;
            if (!node->contains(key, index))
                return false;

            if (!level) {
                entry = &node->m_entries.entries[index];
                return true;
            }

            node = node->m_entries.children[index];
        }

        return false;
    }

    static bool find(node_type *root, uint64_t key, T *entry) {
        node_type *node = root;

        while (node) {
            uint8_t level = node->m_level;
            size_t index = 0;
            if (!node->contains(key, index))
                return false;

            if (!level) {
                *entry = node->m_entries.entries[index];
                return true;
            }

            node = node->m_entries.children[index];
        }

        return false;
    }

    /// Get the entry with the given key. If one does not exist yet,
    /// create a new one, insert it, and return that.
    /// \arg root  [inout] The root node of the tree. This pointer may be updated.
    /// \arg key   Key of the entry to find
    /// \returns   A reference to the entry that was found or created
    static T& find_or_add(node_type * &root, uint64_t key) {
        node_type *leaf = nullptr;
        bool existing = false;

        if (!root) {
            // There's no root yet, just make a leaf and make it
            // the root.
            leaf = new node_type(key, 0);
            root = leaf;
        } else {
            // Find or insert an existing leaf
            node_type **parent = &root;
            node_type *node = root;
            while (node) {
                uint8_t level = node->m_level;
                size_t index = 0;
                if (!node->contains(key, index)) {
                    // We found a valid parent but the slot where this node should
                    // go contains another node. Insert an intermediate parent of
                    // this node and a new leaf into the parent.
                    uint64_t other = node->m_base;
                    uint8_t lcl = max_level + 1;
                    while (index_for(key, lcl) == index_for(other, lcl)) --lcl;

                    node_type *inter = new node_type(key, lcl);

                    size_t other_index = index_for(other, lcl);
                    inter->m_entries.children[other_index] = node;
                    *parent = inter;

                    leaf = new node_type(key, 0);
                    size_t key_index = index_for(key, lcl);
                    inter->m_entries.children[key_index] = leaf;
                    break;
                }

                if (!level) {
                    leaf = node;
                    break;
                }

                parent = &node->m_entries.children[index];
                node = *parent;
            }

            assert( (node || parent) &&
                    "Both node and parent were null in find_or_add");

            if (!node) {
                // We found a parent with an empty spot where this node should
                // be. Insert a new leaf there.
                leaf = new node_type(key, 0);
                *parent = leaf;
            }
        }

        assert(leaf && "Got through find_or_add without a leaf");

        uint8_t index = index_for(key, 0);
        return leaf->m_entries.entries[index];
    }

    virtual ~radix_tree() {
        if (m_level) {
            for (auto &c : m_entries.children)
                delete c;
        }
    }

public:
    constexpr static size_t bits_per_level = util::const_log2(N);

protected:
    static inline size_t level_shift(uint8_t level) { return level * bits_per_level + extra_shift; }
    static inline size_t level_mask(uint8_t level)  { return -1ull << level_shift(level); }

    static inline size_t index_for(uint64_t key, uint8_t level) {
        return (key >> level_shift(level)) & (N-1);
    }

    radix_tree(uint64_t base, uint8_t level) :
        m_base {base & level_mask(level+1)},
        m_level {level} {
        memset(&m_entries, 0, sizeof(m_entries));
    }

    /// Check if this node should contain the given key
    /// \arg key    The key being searched for
    /// \arg index  [out] If found, what entry index should contain it
    /// \returns    True if the key is contained
    bool contains(uintptr_t key, size_t &index) const {
        if ((key & level_mask(m_level+1)) != m_base)
            return false;
        index = index_for(key, m_level);
        return true;
    }

    /// The base key of the range this node encompasses
    uint64_t m_base;

    /// Level of this node: Level 0 nodes contain entries, other levels M point
    /// to nodes of level M-1.
    uint8_t m_level;

    union {
        T entries[N];
        node_type *children[N];
    } m_entries;

};

} // namespace util
