#pragma once
/// \file image.h
/// Definition of a class representing a loaded ELF image

#include <stdint.h>
#include <j6/types.h>
#include <util/counted.h>
#include <util/linked_list.h>

#include "symbols.h"

struct dyn_entry;
struct string_table;
struct rela;
struct image_list;

struct image
{
    uintptr_t base;
    const char *name;
    uintptr_t *got;

    string_table strtab;
    util::counted<rela const> jmprel;
    util::counted<rela const> dynrel;

    symbol const *dynsym = nullptr;
    gnu_hash_table const *gnu_hash = nullptr;

    bool relocated = false;
    bool ctors = false;

    /// Look up a string table entry in this image's string table.
    const char * string(unsigned index) const {
        if (index > strtab.count) return nullptr;
        return strtab.pointer + index;
    }

    /// Get the address of the DYNAMIC table
    inline const dyn_entry *dyn_table() const {
        return reinterpret_cast<const dyn_entry*>(got[0] + base);
    }

    void read_dyn_table(dyn_entry const *table = nullptr);

    /// Do all relocation on this image
    void relocate(image_list &ctx);

    /// Do the relocations from a single table
    void parse_rela_table(const util::counted<const rela> &table, image_list &ctx);

    /// Look up a symbol in this image's symbol table, and return an address
    /// if it is defined, or otherwise 0.
    uintptr_t lookup(const char *name) const;
};

struct image_list :
    public util::linked_list<image>
{
    /// Resolve a symbol name to an address, respecting library load order
    uintptr_t resolve(const char *symbol);

    /// Recursively load images and return an image_list
    void load(j6_handle_t vfs_mb, uintptr_t addr);

    /// Find an image with the given name in the list, or return null.
    item_type * find_image(const char *name);
};
