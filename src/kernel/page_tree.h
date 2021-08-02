#pragma once
/// \file page_tree.h
/// Definition of mapped page tracking structure and related definitions

#include <stdint.h>

/// A radix tree node that tracks mapped pages
class page_tree
{
public:
    /// Get the physical address of the page at the given offset.
    /// \arg root    The root node of the tree
    /// \arg offset  Offset into the VMA, in bytes
    /// \arg page    [out] Receives the page physical address, if found
    /// \returns     True if a page was found
    static bool find(const page_tree *root, uint64_t offset, uintptr_t &page);

    /// Get the physical address of the page at the given offset. If one does
    /// not exist yet, allocate a page, insert it, and return that.
    /// \arg root    [inout] The root node of the tree. This pointer may be updated.
    /// \arg offset  Offset into the VMA, in bytes
    /// \arg page    [out] Receives the page physical address, if found
    /// \returns     True if a page was found
    static bool find_or_add(page_tree * &root, uint64_t offset, uintptr_t &page);

private:
    page_tree(uint64_t base, uint8_t level);

    /// Stores the page offset of the start of this node's pages in bits 0:41
    /// and the depth of tree this node represents in bits 42:44 (0-7)
    uint64_t m_base;

    /// For a level 0 node, the entries area all physical page addresses.
    /// Other nodes contain pointers to child tree nodes.
    union {
        uintptr_t entry;
        page_tree *child;
    } m_entries[64];
};
