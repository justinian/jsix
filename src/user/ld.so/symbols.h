#pragma once
/// \file symbols.h
/// Symbol lookup routines and related data structures

#include <stdint.h>
#include <util/counted.h>

class string_table :
    public util::counted<char const>
{
public:
    const char *lookup(size_t offset) const {
        if (offset > count) return nullptr;
        return pointer + offset;
    }
};

struct symbol
{
    uint32_t name;
    uint8_t type : 4;
    uint8_t binding : 4;
    uint8_t _reserved0;
    uint16_t section;
    uintptr_t address;
    size_t size;
};

struct gnu_hash_table
{
    uint32_t bucket_count;
    uint32_t start_symbol;
    uint32_t bloom_count;
    uint32_t bloom_shift;
    uint64_t bloom [0];
};
