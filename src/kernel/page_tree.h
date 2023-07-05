#pragma once
/// \file page_tree.h
/// Definition of mapped page tracking structure and related definitions

#include <stdint.h>

#include <arch/memory.h>
#include <util/radix_tree.h>

/// A radix tree node that tracks mapped pages
class page_tree :
    public util::radix_tree<uintptr_t, 64, 5, arch::frame_bits>
{
public:
    /// Get the physical address of the page at the given offset. If one does
    /// not exist yet, allocate a page, insert it, and return that. Overrides
    /// `util::radix_tree::find_or_add`.
    /// \arg root    [inout] The root node of the tree. This pointer may be updated.
    /// \arg offset  Offset into the VMA, in bytes
    /// \arg page    [out] Receives the page physical address, if found
    /// \returns     True if a page was found
    static bool find_or_add(page_tree * &root, uint64_t offset, uintptr_t &page);

    /// Add an existing mapping not allocated via find_or_add.
    /// \arg root    [inout] The root node of the tree. This pointer may be updated.
    /// \arg offset  Offset into the VMA, in bytes
    /// \arg page    The mapped page physical address
    static void add_existing(page_tree * &root, uint64_t offset, uintptr_t page);
};
