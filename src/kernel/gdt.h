#pragma once
/// \file gdt.h
/// Definitions relating to a CPU's GDT table

#include <stdint.h>
#include <util/bitset.h>

class TSS;

class GDT
{
public:
    GDT(TSS *tss);

    /// Get the currently running CPU's GDT
    static GDT & current();

    /// Install this GDT to the current CPU
    void install() const;

    /// Get the addrss of the pointer
    inline const void * pointer() const { return static_cast<const void*>(&m_ptr); }

    /// Dump debug information about the GDT to the console.
    /// \arg index  Which entry to print, or -1 for all entries
    void dump(unsigned index = -1) const;

    enum class type
    {
        accessed,
        read_write,
        conforming,
        execute,
        system,
        ring1,
        ring2,
        present
    };

private:
    void set(uint8_t i, uint32_t base, uint64_t limit, bool is64, util::bitset8 t);
    void set_tss(TSS *tss);

    struct descriptor
    {
        uint16_t limit_low;
        uint16_t base_low;
        uint8_t base_mid;
        util::bitset8 type;
        uint8_t size;
        uint8_t base_high;
    } __attribute__ ((packed, align(8)));

    struct ptr
    {
        uint16_t limit;
        descriptor *base;
    } __attribute__ ((packed, align(4)));

    descriptor m_entries[8];
    TSS *m_tss;

    ptr m_ptr;
};
