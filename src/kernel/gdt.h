#pragma once
/// \file gdt.h
/// Definitions relating to system descriptor tables: GDT, IDT, TSS
#include <stdint.h>
#include "kutil/memory.h"

/// Set up the GDT and TSS, and switch segment registers to point
/// to them.
void gdt_init();

/// Set an entry in the IDT
/// \arg i         Index in the IDT (vector of the interrupt this handles)
/// \arg addr      Address of the handler
/// \arg selector  GDT selector to set when invoking this handler
/// \arg flags     Descriptor flags to set
void idt_set_entry(uint8_t i, uint64_t addr, uint16_t selector, uint8_t flags);

/// Set the stack pointer for a given ring in the TSS
/// \arg ring  Ring to set for (0-2)
/// \arg rsp   Stack pointer to set
void tss_set_stack(int ring, addr_t rsp);

/// Get the stack pointer for a given ring in the TSS
/// \arg ring  Ring to get (0-2)
/// \returns   Stack pointers for that ring
addr_t tss_get_stack(int ring);

/// Dump information about the current GDT to the screen
void gdt_dump();

/// Dump information about the current IDT to the screen
void idt_dump();
